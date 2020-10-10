import sqlite3

#We need a data base that stores position X,Y,Width,Height
start_db= "__HOME__/databases/start.db"    
alldata_db = "__HOME__/databases/alldata.db"
conn = sqlite3.connect(alldata_db)
c = conn.cursor()

def request_handler(request):
    if request['method'] == 'POST':
        passin = str(request["form"]["pass"])
        username = str(request["form"]["user"])
        startconn = sqlite3.connect(start_db)
        s = startconn.cursor()
        s.execute('''CREATE TABLE IF NOT EXISTS startPass (user text, pass text);''')
        s.execute('''INSERT into startPass VALUES (?,?);''', (username, passin))
        vals = s.execute('''SELECT * from startPass;''').fetchall()
        startconn.commit()
        startconn.close()
        return vals

    if request['method'] == 'GET':
        # vals = c.execute('''SELECT * FROM blockPosition;''').fetchall()
        conn.close()
        # return vals
        startconn = sqlite3.connect(start_db)
        s = startconn.cursor()
        s.execute('''CREATE TABLE IF NOT EXISTS startPass (user text, pass text);''')
        vals = s.execute('''SELECT * from startPass;''').fetchone()[0]
        startconn.close()
        return vals