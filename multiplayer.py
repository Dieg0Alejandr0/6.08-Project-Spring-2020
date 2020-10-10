import sqlite3
import random

#We need a data base that stores position X,Y,Width,Height
blockPosition_db = "__HOME__/databases/blockPosition.db"
JSblockPosition_db = "__HOME__/databases/JSblockposition.db"
start_db= "__HOME__/databases/start.db"
alldata_db = "__HOME__/databases/alldata.db"

allconn = sqlite3.connect(alldata_db)
JSconn = sqlite3.connect(JSblockPosition_db)
conn = sqlite3.connect(blockPosition_db)
startconn = sqlite3.connect(start_db)

a = allconn.cursor()
s = startconn.cursor()
c = conn.cursor()
d = JSconn.cursor()

a.execute('''CREATE TABLE IF NOT EXISTS blockPosition (xCoord int, yCoord int, height int, width int, need int);''')
d.execute('''CREATE TABLE IF NOT EXISTS JSblockPosition (xCoord int, yCoord int, height int, width int, need int)''')
c.execute('''CREATE TABLE IF NOT EXISTS blockPosition (xCoord int, yCoord int, height int, width int, need int);''')
s.execute('''CREATE TABLE IF NOT EXISTS startPass (user text, pass text);''')

def placeGood(x,y,h,w):
    allBlocks = list(a.execute('''SELECT * FROM blockPosition WHERE need = 1'''))
    if(allBlocks is None):
        return True
    if(len(allBlocks)==0):
        return True 
    for placedBlock in allBlocks:
        placedX, placedY, placedH, placedW, dummy = placedBlock
        placedX = int(placedX)
        placedY = int(placedY)
        placedW = int(placedW)
        placedH = int(placedH)
        if(x + w <= placedX or x >= placedX+ placedW or y >= placedY + placedH or y + h<= placedY):
            continue 
        else:
            return False
    return True

def request_handler(request):

    # Handle the browser GET request by returning the webpage
    if request['method'] == 'GET':
        try:
            allPass = [job[0] for job in s.execute("SELECT pass FROM startPass")]
            allPass.append("swag")
            username1 = s.execute('''SELECT * from startPass;''').fetchone()[0]
            userlist = []
            userlist.append(username1)
            if request['values']['pass'] in allPass: 
                s.execute('''DELETE FROM startPass''')
                startconn.commit()
                startconn.close()
                with open('__HOME__/finalProject/multiplayer.htm', 'r') as myfile:
                    htmlData = myfile.read()
                # Update HTML with random coordinate points
                #newHTML = htmlData.format(*userlist)
                newHTML = htmlData
                return newHTML
            else:
                with open('__HOME__/finalProject/failscreen.htm', 'r') as myfile:
                    return myfile.read()
        except:
            try:
                if request['values']['pass']=="swag": 
                    s.execute('''DELETE FROM startPass''')
                    startconn.commit()
                    startconn.close()
                    with open('__HOME__/finalProject/multiplayer.htm', 'r') as myfile:
                        htmlData = myfile.read()
                    # Update HTML with random coordinate points
                    #newHTML = htmlData.format(*userlist)
                    newHTML = htmlData
                    return newHTML
                else:
                    with open('__HOME__/finalProject/failscreen.htm', 'r') as myfile:
                        return myfile.read()
            except:
                with open('__HOME__/finalProject/failscreen.htm', 'r') as myfile:
                    return myfile.read()
        # allPass.append("swag")
        # username1 = s.execute('''SELECT * from startPass;''').fetchone()[0]
        # userlist = []
        # userlist.append(username1)
        # if request['values']['pass'] in allPass: 
        #     s.execute('''DELETE FROM startPass''')
        #     startconn.commit()
        #     startconn.close()
        #     with open('__HOME__/finalProject/multiplayer.htm', 'r') as myfile:
        #         htmlData = myfile.read()
        #     # Update HTML with random coordinate points
        #     #newHTML = htmlData.format(*userlist)
        #     newHTML = htmlData
        #     return newHTML
        # else:
        #     with open('__HOME__/finalProject/failscreen.htm', 'r') as myfile:
        #         return myfile.read()

    # POST requests from button submissions
    if request["method"] == "POST":
        # POST request with dimension information
        if 'dims' in request["form"].keys():
            if request["form"]["dims"][0] == 'f':
                stringDim = request["form"]["dims"]
                dim = eval(stringDim[1:])
                inputNoCoord = (0,0,dim[1],dim[0],0)
                # insert row into our DB with dimension info
                c.execute('''INSERT into blockPosition VALUES (?,?,?,?,?);''', inputNoCoord)
                d.execute('''INSERT into JSblockPosition VALUES (?,?,?,?,?);''', inputNoCoord)
                a.execute('''INSERT into blockPosition VALUES (?,?,?,?,?);''', inputNoCoord)

                JSconn.commit()
                JSconn.close()
                
                allconn.commit()
                allconn.close()

                startconn.commit()
                startconn.close()

                conn.commit()
                conn.close()
        else:
            x = request["form"]["x"]
            y = request["form"]["y"]
            if int(y) >= 35 and int(y) <= 120:
                coord = (x,y)
                h = list(d.execute("""SELECT height FROM JSblockPosition WHERE need = 0 """))[0][0]
                w = list(d.execute("""SELECT width FROM JSblockPosition WHERE need = 0 """))[0][0]
                if placeGood(int(x),int(y),int(h),int(w)):
                    #delete any rows that have already been handled
                    sql_delete_query = """DELETE from blockPosition where need = 1"""
                    JSsql_delete_query = """DELETE from JSblockPosition where need = 1"""

                    c.execute(sql_delete_query)
                    d.execute(JSsql_delete_query)

                    # insert xCoord and yCoord into the row, update need to 1
                    sqlex = '''UPDATE blockPosition SET xCoord = ?, yCoord = ?, need = 1 WHERE need = 0'''
                    JSsqlx = '''UPDATE JSblockPosition SET xCoord = ?, yCoord = ?, need = 1 WHERE need = 0'''

                    d.execute(JSsqlx, (coord[0],coord[1]))
                    c.execute(sqlex,(coord[0],coord[1]))
                    a.execute(sqlex,(coord[0],coord[1]))

                JSconn.commit()
                JSconn.close()

                conn.commit()
                conn.close()

                allconn.commit()
                allconn.close()

                startconn.commit()
                startconn.close()
            else:
                JSconn.close()
                conn.close()
                startconn.close()
                allconn.close()


        with open('__HOME__/finalProject/multiplayer.htm', 'r') as myfile:
            htmlData = myfile.read()
        # newHTML = htmlData.format(*formatRand)
        newHTML = htmlData
        return newHTML
