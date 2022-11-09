#include "mDNSResolver.h"
#include "Arduino.h"
#include "Particle.h"

#ifdef TEST
#include <stdlib.h>
#endif

namespace mDNSResolver {
  Cache cache;

  Resolver::Resolver(UDP& udp) {
    timeout = 0;
    this->udp = udp;
  }

  Resolver::Resolver(UDP& udp, IPAddress localIP) {
    timeout = 0;
    this->udp = udp;
    this->localIP = localIP;
  }

  Resolver::~Resolver() {}

  void Resolver::setLocalIP(IPAddress localIP) {
    this->localIP = localIP;
  }

  IPAddress Resolver::search(const char* name) {
	 // Serial.println("er");
    if(!isMDNSName(name)) {
	Serial.println("not valid");
      return INADDR_NONE;
    }

    cache.expire();

    int attempts = 0;

    while(attempts < MDNS_ATTEMPTS) {
      int index = cache.search(name);
      if(index == -1) {
        cache.insert(Response(name, 5));			//ttl = 5
        continue;
      } else if(cache[index].resolved) {
		  // Serial.println("Found " + cache[index].ipAddress);
		  udp.leaveMulticast(MDNS_BROADCAST_IP);
        return cache[index].ipAddress;
      }

	// Serial.println("MOMOMO");
      long now = millis();

      // Send a query packet every second
      if(now - timeout > MDNS_RETRY) {
        query(name);
        timeout = now;
        attempts++;
      }

      MDNS_RESULT result = read();
      if(result != E_MDNS_OK) {
		  // Serial.println("Search return 1");
        return INADDR_NONE;
      }
    }
	udp.leaveMulticast(MDNS_BROADCAST_IP);
	init = false;
	// Serial.println("Search return 2");
    return INADDR_NONE;
  }

  void Resolver::query(const char* name) {
	  // Serial.println("Query");
    Query query(name);
    udp.beginPacket(MDNS_BROADCAST_IP, MDNS_PORT);		//Peet
    query.sendPacket(udp);
    udp.endPacket();
  }

  void Resolver::loop() {
    // Clear the buffers out.
    // This wouldn't be needed if the UDP library had the ability to
    // leave a multicast group
    unsigned int len = udp.parsePacket();
    unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * len);
	// Serial.println("mDNS1");
    if(buffer != NULL) {
      udp.read(buffer, len);
      free(buffer);
    }
	// Serial.println("mDNS2");
  }

  MDNS_RESULT Resolver::read() {
    cache.expire();

    if(!init) {
      init = true;
	  Serial.println("Begin Multicast");
	  udp.begin(5353);
      Serial.println(udp.joinMulticast(MDNS_BROADCAST_IP));	  //Peet
    }

    unsigned int len = udp.parsePacket();

    if(len > 0) {
      unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * len);

      if(buffer == NULL) {
        // Out of memory - the packet is probably too big to parse. Probably.
        // Silently bombing out, possibly isn't great, but it'll do for the moment.
		Serial.println("Out of memory");
        return E_MDNS_OUT_OF_MEMORY;
      }

      udp.read(buffer, len);
	  udp.flush();
      lastResult = Answer::process(buffer, len, cache);

      free(buffer);
	  // Serial.println("Read return 1");
      return lastResult;
    }
	// Serial.println("Read return 2");
	// init = false;
    return E_MDNS_OK;
  }

  bool Resolver::isMDNSName(const char* name) {
    int len = strlen(name);
    int tldLen = strlen(MDNS_TLD);
	
	// Serial.println("LOll");
    if(len < tldLen) {
      return false;
    }
	// Serial.println("LOLO");
    return strcmp(name + len - tldLen, MDNS_TLD) == 0;
  }
};
