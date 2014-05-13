# 1 "bf561sphinx_heaptab.c"
 








































 


extern "asm" int ldf_heap_space;
extern "asm" int ldf_heap_length;
extern "asm" int MyHeap_space;
extern "asm" int MyHeap_length;


struct heap_table_t
{
  void          *base;
  unsigned long  length;
  long int       userid;
};

#pragma file_attr("libData=HeapTable")
#pragma section("constdata")
struct heap_table_t heap_table[3] =
{


  { &ldf_heap_space, (int) &ldf_heap_length, 0 },
  { &MyHeap_space, (int) &MyHeap_length, 1 },


  { 0, 0, 0 }
};


