#!/usr/bin/env octave

figure;
hold on;
cdf("udping.csv", 'r');
cdf("udping_prison.csv", 'b');
legend("direct:udp","prison:udp");
xlabel("[usec]");
ylabel("[percentage:%]");
