import sqlite3
import json
blockPosition_db = "__HOME__/databases/JSblockposition.db"
conn = sqlite3.connect(blockPosition_db)
c = conn.cursor()

def request_handler(request):
    if request['method'] == 'GET':
        # find the entries where they have had coords inserted (need = 1)
        try:
            blockInfo = c.execute('''SELECT * FROM JSblockPosition;''').fetchone()
            sql_delete_query = """DELETE from JSblockPosition where need = 1"""
            c.execute(sql_delete_query)
            conn.commit()
            conn.close()
            if blockInfo is None:
                retDict = {
                'rect': [0,0,0,0]
                }
                return json.dumps(retDict)
            retDict = {
                'rect': list(blockInfo)
            }
            retjson = json.dumps(retDict)
            return retjson 
        except:
            conn.close()
            retDict = {
                'rect': [0,0,0,0]
                }
            return json.dumps(retDict)