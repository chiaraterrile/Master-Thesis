import time

f = open('log_RT.txt', 'r')
content = f.read()
f.close()

content_list = list(content.split("\n"))

new_log = []
for i in range (0,int(len(content_list)/40)+1) :
    print(' I am writing')
    list_to_send = content_list[40*i : 40*i +40]
    new_log = new_log + list_to_send
    with open("log.txt", "w") as text_file:
        text_file.write('\n'.join(map(str, new_log)))
    #time.sleep(0.00005)
    


print('DONE')