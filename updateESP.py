import sqlite3

blockPosition_db = "__HOME__/databases/blockPosition.db"
conn = sqlite3.connect(blockPosition_db)
c = conn.cursor()

def request_handler(request):
    if request['method'] == 'GET':
        # find the entries where they have had coords inserted (need = 1)
        try:
            blockInfo = c.execute('''SELECT * FROM blockPosition WHERE need = 1;''').fetchone()
        except:
            conn.close()
            return None
        # get rid of those entries
        sql_delete_query = """DELETE from blockPosition where need = 1"""
        c.execute(sql_delete_query)
        conn.commit()
        conn.close()
        # we did not have an entry
        if blockInfo is None:
            return "0,0,0,0"
        blockInfo = list(blockInfo)[:-1]
        realBlockInfo = str(blockInfo)[1:-1]
        realBlockInfo.replace(' ', '')
        return realBlockInfo