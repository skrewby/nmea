# Development

## Pre-commit

Install [Pre-Commit](https://pre-commit.com/) or [Prek](https://github.com/j178/prek) and run the following:

```sh
pre-commit install
```

or

```sh
prek install
```

In the project root directory.

## Testing with VCAN

Create the vcan0 link

```sh
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Run the listener example with

```sh
./run.sh run
```

In another, send CAN frames with

```sh
cansend vcan0 123#DEADBEEF
```
