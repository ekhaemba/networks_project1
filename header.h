struct Header {
  unsigned short sequence;
  unsigned short count;
};
struct HeaderUDP
{
  unsigned short sequence;
  unsigned short count;
  char data[1024];	
};