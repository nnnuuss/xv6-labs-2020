// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define MAX 13
#define freeid 13
struct {
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct hashnode{
    struct spinlock lock; //每条散列链表上一个锁
    struct buf head;
  }hashtable[MAX];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < MAX; ++i)
  initlock(&bcache.hashtable[i].lock, "bcache");

  // Create linked list of buffers
  for (int i = 0; i < MAX; ++i){  //初始化每条散列链表
    bcache.hashtable[i].head.prev = &bcache.hashtable[i].head;
    bcache.hashtable[i].head.next = &bcache.hashtable[i].head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){  //初始状态把所有空闲块放在单独一条链表上
    b->next = bcache.hashtable[0].head.next;
    b->prev = &bcache.hashtable[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hashtable[0].head.next->prev = b;
    bcache.hashtable[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint id = blockno%MAX;
  acquire(&bcache.hashtable[id].lock);

  // Is the block already cached?
  for(b = bcache.hashtable[id].head.next; b != &bcache.hashtable[id].head; b = b->next){  //search buf
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hashtable[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.hashtable[id].head.prev; b != &bcache.hashtable[id].head; b = b->prev){  //找到空闲块
    if (b->refcnt == 0){
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.hashtable[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  for (int i = 0; i < MAX; ++i){
    if (i==id)continue;
    acquire(&bcache.hashtable[i].lock);
    for (b = bcache.hashtable[i].head.prev; b != &bcache.hashtable[i].head; b = b->prev){
      if (b->refcnt == 0){
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->prev->next = b->next;
        b->next->prev = b->prev;
        b->next = bcache.hashtable[id].head.next; //将节点插入id链中
        b->prev = &bcache.hashtable[id].head;
        bcache.hashtable[id].head.next->prev = b;
        bcache.hashtable[id].head.next = b;
        release(&bcache.hashtable[i].lock);
        release(&bcache.hashtable[id].lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.hashtable[i].lock);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;
  // printf("bread\n");
  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  // printf("write\n");
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint id = b->blockno%MAX;
  acquire(&bcache.hashtable[id].lock);
  b->refcnt--;
  release(&bcache.hashtable[id].lock);
}

void
bpin(struct buf *b) {
  uint id = b->blockno%MAX;
  acquire(&bcache.hashtable[id].lock);
  b->refcnt++;
  release(&bcache.hashtable[id].lock);
}

void
bunpin(struct buf *b) {
  uint id = b->blockno%MAX;
  acquire(&bcache.hashtable[id].lock);
  b->refcnt--;
  release(&bcache.hashtable[id].lock);
}