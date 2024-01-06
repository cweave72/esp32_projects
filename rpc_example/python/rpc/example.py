import logging
from rich.logging import RichHandler
from rich import inspect

from protorpc import build_api
from protorpc.connection.udp_connection import UdpConnection

from rpc.lib import RpcFrame

logger = logging.getLogger()


def main():

    conn = UdpConnection(addr='192.168.1.190', port=13000)
    conn.connect()

    api = build_api(RpcFrame, conn)
    api['test_callset'].add(a=10, b=20)
    api['test_callset'].handlererror()
    api['test_callset'].setstruct(
        var_int32=-55,
        var_uint32=40001230,
        var_int64=0x0000beefbeefbeef,
        var_uint64=0x55,
        var_bool=True,
        var_uint32_array=[0, 1, 23, 4, 5, 6, 8, 123456789],
        var_string="hello world, th",
        var_bytes=b'asdfjkl;'
    )

    conn.close()

    #frame = RpcFrame()
    #header = frame.header
    #header.seqn = 1

    #test = frame.test_callset
    #test.add_call.a = 10
    #test.add_call.b = 20

    #logger.info("frame:")
    #inspect(frame, all=False)

    #ser = frame.SerializeToString()
    #logger.info(f"serialized={ser}")

    #r_frame = RpcFrame()
    #r_frame.parse(ser)
    #logger.info("r_frame:")
    #inspect(r_frame)


if __name__ == "__main__":
    logger.setLevel(logging.DEBUG)
    ch = RichHandler(rich_tracebacks=True, show_time=False)
    ch.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    main()
