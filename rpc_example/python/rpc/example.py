import sys
import logging
from rich.logging import RichHandler
from rich import inspect
from rich.console import Console

from protorpc import build_api
from protorpc.connection.udp_connection import UdpConnection

from rpc.lib import RpcFrame
from rtosutils import RtosUtils

logger = logging.getLogger()


def main(ip):

    conn = UdpConnection(addr=ip, port=13000)
    conn.connect()

    api = build_api(RpcFrame, conn)

    reply = api['test_callset'].add(a=10, b=20)
    logger.info(f"result: sum={reply.result.sum}")

    reply = api['test_callset'].handlererror()
    logger.info(f"result={reply.result}")

    reply = api['test_callset'].setstruct(
        var_int32=-55,
        var_uint32=40001230,
        var_int64=0x0000beefbeefbeef,
        var_uint64=0x55,
        var_bool=True,
        var_uint32_array=[0, 1, 23, 4, 5, 6, 8, 123456789],
        var_string="hello world, th",
        var_bytes=b'asdfjkl;',
        no_reply=False
    )
    logger.info(f"result={reply.result}")

    rtos = RtosUtils(api)
    tbl = rtos.get_system_tasks_table()
    con = Console()
    con.print(tbl)

    #reply = api['rtosutils_callset'].get_system_tasks()
    #logger.info(f"result={reply.result}")

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
    logger.setLevel(logging.INFO)
    ch = RichHandler(rich_tracebacks=True, show_time=False)
    ch.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    main(sys.argv[1])
