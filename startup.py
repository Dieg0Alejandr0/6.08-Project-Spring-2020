def request_handler(request):
    with open('__HOME__/finalProject/startPrompt.htm', 'r') as myfile:
        return myfile.read()