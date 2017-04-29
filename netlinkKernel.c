// More useful information:
// https://kernelnewbies.org/FAQ/Hashtables
// http://lxr.free-electrons.com/source/include/linux/hashtable.h?v=<version of kernel>

#include <linux/hashtable.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_USER 31

struct incoming_msg {
  u32 request; // 0: read an object and delete if found.
               // 1: insert an object to table.
  u32 key1;
  u32 key2;
  u32 val;
};

struct outgoing_msg {
  u32 status; // 0: read failed (object not found);
              // 1: read succeeded;
              // 2: write finished (remaining data is an echo).
  u32 key1;
  u32 key2;
  u32 val;
};

struct hashed_object {
  u32 key1;
  u32 key2;
  u32 val;
  struct hlist_node hash_list;
};

struct sock* nl_sk = NULL;

DEFINE_HASHTABLE(hash_table, 8); // 8 bits -> 256 buckets

static void nl_recv_msg(struct sk_buff* skb) {

  struct nlmsghdr* nlh;
  struct incoming_msg* in;
  struct outgoing_msg out;
  int pid;
  struct sk_buff* skb_out;

  nlh = (struct nlmsghdr*) skb->data;
  in = (struct incoming_msg*) nlmsg_data(nlh);

  if (in->request == 0) { // read and delete
    u32 hash_key = (in->key1) ^ (in->key2);
    struct hashed_object* object;
    out.status = 0;
    out.key1 = in->key1;
    out.key2 = in->key2;
    hash_for_each_possible(hash_table, object, hash_list, hash_key) {
      if (object->key1 == in->key1 && object->key2 == in->key2) {
        out.status = 1;
        out.val = object->val;
        hash_del(&(object->hash_list));
        kfree(object);
        break;
      }
    }
  } else if (in->request == 1) {
    // insert (in this implementation, entries with duplicated IDs are allowed)
    struct hashed_object* object = kmalloc(sizeof(struct hashed_object), GFP_KERNEL);
    object->key1 = in->key1;
    object->key2 = in->key2;
    object->val = in->val;
    u32 hash_key = (in->key1) ^ (in->key2);
    hash_add(hash_table, &(object->hash_list), hash_key);
    out.status = 2;
    out.key1 = in->key1;
    out.key2 = in->key2;
    out.val = in->val;
  } else {
    printk(KERN_INFO "Message received via netlink but argument is invalid.\n");
    return;
  }

  pid = nlh->nlmsg_pid;
  skb_out = nlmsg_new(sizeof(out), 0);
  nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, sizeof(out), 0);
  NETLINK_CB(skb_out).dst_group = 0;
  memcpy(nlmsg_data(nlh), &out, sizeof(out));
  nlmsg_unicast(nl_sk, skb_out, pid);
}

static int __init netlink_hashtable_init(void) {
  struct netlink_kernel_cfg cfg = {
    .input = nl_recv_msg
  };
  nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
  return 0;
}

static void __exit netlink_hashtable_exit(void) {
  netlink_kernel_release(nl_sk);
}

module_init(netlink_hashtable_init);
module_exit(netlink_hashtable_exit);

MODULE_LICENSE("GPL");