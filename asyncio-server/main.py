import argparse
import asyncio
import atexit
import datetime as dt
import json
import sqlite3

CREATE_TABLE = """CREATE TABLE IF NOT EXISTS sensor_readings (
                      id INTEGER PRIMARY KEY,
                      event_time TEXT NOT NULL,
                      payload JSONB NOT NULL);
               """

INSERT = """INSERT INTO sensor_readings (event_time, payload) VALUES (?, ?)"""

def adapt_datetime_iso(val):
    """Adapt datetime.datetime to timezone-naive ISO 8601 date."""
    return val.replace(tzinfo=None).isoformat()

def convert_datetime(val):
    """Convert ISO 8601 datetime to datetime.datetime object."""
    return dt.datetime.fromisoformat(val.decode())

def try_parse(value):
    try:
        return int(value)
    except:
        pass
        
    try:
        return float(value)
    except:
        pass

    return value

async def handle_conn(db_conn, reader, writer):
    addr = writer.get_extra_info('peername')
    sock_name = writer.get_extra_info('sockname')
    print("connection opened on {} by {}".format(sock_name, addr))
    kv_pairs = dict()
    while True:
        data_bytes = await reader.readline()
        message = data_bytes.decode().strip()
        if not message:
            break
        if message == "end":
            # do something with kv_pairs
            cursor = db_conn.cursor()
            now = dt.datetime.now()
            kv_pairs["recieved_timestamp"] = now.isoformat()
            cursor.execute(INSERT, (now, json.dumps(kv_pairs)))
            db_conn.commit()
            print(kv_pairs)
            kv_pairs = dict()
        else:
            idx = message.find("\t")
            # just skip bad lines
            if idx == -1:
                print("Received a bad line: '{}'".format(message))
                continue
            else:
                key = message[:idx].strip()
                value = message[idx + 1:].strip()
                kv_pairs[key] = try_parse(value)

    print("connection on {} closed by {}".format(sock_name, addr))
    writer.close()
    await writer.wait_closed()

def create_handle_func(db_conn):
    async def wrapper(reader, writer):
        await handle_conn(db_conn, reader, writer)
    return wrapper

async def main(host, port, db_conn):
    handle_func = create_handle_func(db_conn)
    server = await asyncio.start_server(handle_func, host, port)

    async with server:
        await server.serve_forever()

def open_database(dbname):
    conn = sqlite3.connect(dbname)
    cursor = conn.cursor()
    cursor.execute(CREATE_TABLE)
    conn.commit()

    atexit.register(conn.close)

    return conn

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", type=str, required=True)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--dbname", type=str, required=True)

    args = parser.parse_args()

    sqlite3.register_adapter(dt.datetime, adapt_datetime_iso)
    sqlite3.register_converter("datetime", convert_datetime)

    db_conn = open_database(args.dbname)
    
    asyncio.run(main(args.host, args.port, db_conn))
