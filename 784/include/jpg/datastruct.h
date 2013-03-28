
/*             _      | |        |  _      | |      |  | |
 *            / \     | |          / \     | |  /|  |  | |             \
 *      Q___|____\  __| | | . |__Q____\  __| | (_|__|__| |  Q_|__|__|___)
 *  ___/    :      /      |___|         /               ___/          .
 *               _/                   _/
 *
 */

//  Written by Hamed Ahmadi Nejad
//    ahmadinejad@ce.sharif.edu
//    comphamed@yahoo.com

#ifndef datastruct_h
#define datastruct_h

template <class T>
struct LList {
  int nelem;
  struct entry {
    T x;
    entry *prev, *next;
  } *head, *tail;
  entry *first() const { 
    return head->next;
  }
  entry *last() const {
    return tail->prev;
  }
  LList() {
    head=new entry; tail=new entry;
    //head->x=tail->x=0;
    head->next=tail; tail->prev=head;
    head->prev=tail->next=0;
    nelem=0;
  }
  ~LList() {
    clear();
    delete head; delete tail;
  }
  void addtofront(T z) {  
    entry *e=new entry;
    e->x=z;
    e->prev=head; e->next=head->next;
    head->next->prev=e; head->next=e;    
    nelem--;
  }
  void append(T z) { 
    entry *e=new entry;
    e->x=z;
    e->next=tail; e->prev=tail->prev;
    tail->prev->next=e; tail->prev=e;
    nelem++;
  }
  void kill(entry *e) {
    e->next->prev=e->prev; e->prev->next=e->next;
    delete e;
    nelem--;
  }
  void clear() {
    entry *e;
    while ((e=first())->next) kill(e);
    nelem=0;
  }
  void operator = (const LList <T> &o) {
    clear();
    for (entry *e=o.first(); e->next; e=e->next)
      append(e->x);
  }
};

template <class T>
struct Heap {  //A minheap
  T *a;
  int n;
  void make() {
    for (int i=n/2;i;i--)
      siftdown(i);
  }
  void siftdown(int x) {
    int v; 
    for(;;) {
      v=x;
      if (2*x<=n && a[2*x]<a[v]) v=2*x;
      if (2*x<n && a[2*x+1]<a[v]) v=2*x+1;
      if (v==x) break;
      T t=a[x]; a[x]=a[v]; a[v]=t;
      x=v;
    }
  }
  T extractmin() {
    T t=a[1];
    a[1]=a[n];
    n--;
    siftdown(1);
    return t;
  }
  Heap (int size) {
    a=new T[size+2];
    n=size;
  }
  ~Heap () {
    delete [] a;
  }
};


#endif
