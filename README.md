# coremidi_latency_test

_The accuracy you crave, the nanoseconds you deserve!_

coremidi_latency_test is a macos command line program that allows you to 
test the io latency of your midi interfaces in nanoseconds.

## Usage
--------

```
make
```
then:
```
./coremidi_latency_test
```
without args will list interface i/o ports by number,
choose your desired output/input port from the list, then:
```
./coremidi_latency_test <input port number> <output port number>
```
