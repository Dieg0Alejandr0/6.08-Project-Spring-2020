import sqlite3
import os

leaderboard_db = "__HOME__/databases/leaderboard.db" # just come up with name of database
alldata_db = "__HOME__/databases/alldata.db"

conn = sqlite3.connect(leaderboard_db)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS userScore (userName text, score int);''')

allconn = sqlite3.connect(alldata_db)
d = allconn.cursor()


def weaveData(usernames, scores):
    weavedData = []
    for i in range(len(usernames)):
        weavedData.extend([usernames[i],scores[i]])
    return weavedData

def request_handler(request):
    if request["method"] == "GET":
        d.execute('''DELETE FROM blockPosition''')
        commandUser = "SELECT userName FROM userScore ORDER BY score DESC;"
        commandScore = "SELECT score FROM userScore ORDER BY score DESC;"
        allUsersProto = list(c.execute(commandUser))
        allScoresProto = list(c.execute(commandScore))

        allUsers = []
        allScores = []
        if allUsersProto is None:
            allUsersProto = []
        if allScoresProto is None:
            allScoresProto = []

        for val in allUsersProto:
            allUsers.append(val[0])
        for oVal in allScoresProto:
            allScores.append(oVal[0])

        if len(allUsers) < 5:
            for _ in range(len(allUsers),5):
                allScores.append(0)
                allUsers.append('Nobody yet :/')
        else:
            allUsers = allUsers[:5]
            allScores = allScores[:5]

        rankingData = weaveData(allUsers, allScores)
        conn.commit()
        conn.close()
        allconn.commit()
        allconn.close()

        with open('__HOME__/finalProject/leaderboard.htm', 'r') as myfile:
            unfilledData = myfile.read()
    
        filledData = unfilledData.format(*rankingData)
        return filledData
    
    if request["method"] == "POST":
        user = request["form"]["user"]
        score = round(float(request["form"]["score"]))
        d.execute('''DELETE FROM blockPosition''')

        

        if score == -1:
            bestScore = list(c.execute('''SELECT MAX(score) FROM userScore WHERE userName = ?''', (user,)))[0][0]
            if bestScore is None:
                bestScore = 0
            allconn.commit()
            allconn.close()
            conn.commit()
            conn.close()
            return bestScore

        c.execute('''INSERT into userScore VALUES (?,?);''', (user, score))

        allconn.commit()
        allconn.close()

        conn.commit()
        conn.close()
        return 'Score Logged!'
