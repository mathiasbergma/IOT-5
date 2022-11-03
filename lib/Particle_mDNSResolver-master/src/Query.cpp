#include "Query.h"

namespace mDNSResolver {
  Query::Query(const char* name) {
    this->name = name;
  }

  Query::~Query() {}

  // If we pass in a UDP proxy, we can dynamically allocate the
  // memory without fee of fragmentation, and don't risk losing
  // the reference if this object disappears
  
  // uint8_t qHeader[]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// uint8_t qTypeClass[] = { 0x00, 0x00, 0x01, 0x00, 0x01 };
// int sendQuery(const char* arg) { // takes the name of the requested device
  // uint8_t len = strlen(arg);

  // udp.beginPacket(MDNS_ADDRESS, MDNS_PORT);
  // udp.write(qHeader, sizeof(qHeader));
  // udp.write(len);
  // udp.print(arg);
  // udp.write((uint8_t)0x05);
  // udp.print("local");
  // udp.write(qTypeClass, sizeof(qTypeClass));
  // udp.endPacket();
    
  // return len;    
// }
  
  
  void Query::sendPacket(UDP& socket) {
    int bufferLength = strlen(this->name) + 18;
    unsigned char buffer[bufferLength];
    assemblePacket(buffer, bufferLength);
    socket.write(buffer, bufferLength);
	socket.endPacket();
  }

  void Query::assemblePacket(unsigned char *buffer, int bufferLength) {
    for(int i = 0; i < bufferLength; i++) {
      buffer[i] = 0;
    }

    buffer[5] = 0x01;

    int bufferIndex = 12;
    bufferIndex = buildDNSName(buffer, bufferIndex);

    int qtype = 0x01;
    int qclass = 0x01;

    buffer[bufferIndex++] = '\0';
    buffer[bufferIndex++] = (qtype & 0xFF00) >> 8;
    buffer[bufferIndex++] = qtype & 0xFF;
    buffer[bufferIndex++] = (qclass & 0xFF00) >> 8;
    buffer[bufferIndex++] = qclass & 0xFF;
  }

  int Query::buildDNSName(unsigned char *buffer, unsigned int bufferIndex) {
    int wordstart = 0, wordend = 0;

    do {
      if(this->name[wordend] == '.' || this->name[wordend] == '\0') {
        const int wordlen = wordend - wordstart;
        buffer[bufferIndex++] = (unsigned char)wordlen;

        for(int i = wordstart; i < wordend; i++) {
          buffer[bufferIndex++] = this->name[i];
        }

        if(this->name[wordend] == '.') {
          wordend++;
        }
        wordstart = wordend;
      }

      wordend++;
    } while(this->name[wordstart] != '\0');

    return bufferIndex;
  }
};
