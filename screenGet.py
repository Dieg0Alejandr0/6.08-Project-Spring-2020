import sqlite3
import json
visits_db = '__HOME__/databases/blockData.db'
def request_handler(request):
    if request['method'] == 'GET':
        conn = sqlite3.connect(visits_db)
        c = conn.cursor() 
        c.execute('''CREATE TABLE IF NOT EXISTS bricktab (brck text);''') 
        brickstr = c.execute('''SELECT * FROM bricktab;''').fetchone()[0]
        brickstr1 = str(brickstr)[0:6]+str(brickstr)[7:13]
        arr = ['black' if elem == '1' else 'white' for elem in brickstr1]
        c.execute('''CREATE TABLE IF NOT EXISTS balltab (x int, y int);''') 
        arr1 = list(c.execute('''SELECT * FROM balltab;''').fetchone())
        c.execute('''CREATE TABLE IF NOT EXISTS curstab (x int, y int);''') 
        arr2 = list(c.execute('''SELECT * FROM curstab;''').fetchone())
        conn.commit() 
        conn.close()
        retDict = {
            'brick': arr,
            'ball': arr1,
            'cursor': arr2
        }
        retjson = json.dumps(retDict)
        return retjson

    if request['method'] == 'POST':
        bricks = str(request['form']['brickin'])
        xval1 = int(str(request['form']['xval1']))
        xval2 = int(str(request['form']['xval2']))
        yval1 = int(str(request['form']['yval1']))
        yval2 = int(str(request['form']['yval2']))
        conn = sqlite3.connect(visits_db)
        c = conn.cursor() 
        c.execute('''CREATE TABLE IF NOT EXISTS bricktab (brck text);''') 
        c.execute('''DELETE FROM bricktab;''')
        c.execute('''INSERT into bricktab VALUES (?);''', (bricks,))
        #vals = c.execute('''SELECT * FROM bricktab;''').fetchall()
        c.execute('''CREATE TABLE IF NOT EXISTS balltab (x int, y int);''') 
        c.execute('''DELETE FROM balltab;''')
        c.execute('''INSERT into balltab VALUES (?,?);''', (xval1,yval1))
        #vals1 = c.execute('''SELECT * FROM balltab;''').fetchall()
        c.execute('''CREATE TABLE IF NOT EXISTS curstab (x int, y int);''') 
        c.execute('''DELETE FROM curstab;''')
        c.execute('''INSERT into curstab VALUES (?,?);''', (xval2,yval2))
        #vals2 = c.execute('''SELECT * FROM curstab;''').fetchall()
        conn.commit() 
        conn.close()
        return None