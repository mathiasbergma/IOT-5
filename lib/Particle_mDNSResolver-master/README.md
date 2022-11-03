# Particle_mDNSResolver
A library to resolve the host IP over mDNS for the particle photon boards.

Initialize the library with the udp object.
mDNSResolver::Resolver resolver(udp);

Then use this resolver object to search the hostname that you want to resolve:
IPAddress IP = resolver.search(NAME_TO_RESOLVE);

This will save the IP address of the hostname in the variable IP if it exists in that network.

Tested only on Particle photon but should work on other boards as well.
