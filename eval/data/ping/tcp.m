#!/usr/bin/env octave

figure;
hold on;
cdf("tcping.csv", 'r');
cdf("tcping_prison.csv", 'b');
legend("direct:tcp","prison:tcp");
xlabel("[usec]");
ylabel("[percentage:%]");
