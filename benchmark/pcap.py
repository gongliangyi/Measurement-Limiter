import json
import sys

def write_txt(lst):
    with open('line.txt', 'w') as f:
        length = len(lst)
        for i in range(length):
            f.write(str(lst[i])+'\n')



if __name__ == "__main__":
    
    fd = open('latency.txt', mode = 'w',encoding='utf-8')
    record = {}
    send_number = 0
    send_number2 = 0
    send_number3 = 0
    f = open("./receive.json", "r", encoding = "utf-8")
    data = json.load(f)
    ans = []
    send_number1 = 0
    seq_num = []
    time_stamp = []
    for item in data:
        
#        if 'data' not in item['_source']['layers'].keys():
#            continue
        if 'ip' not in item['_source']['layers'].keys():
            continue
        if item['_source']['layers']['ip']['ip.src'] == "155.138.226.26" and item['_source']['layers']['ip']['ip.dst'] == "144.202.27.162":
            send_number1 += 1
        timestamp = eval(item['_source']['layers']['frame']['frame.time_epoch'])
        time_stamp.append(timestamp)
        seq = item['_source']['layers']["tcp"]["tcp.seq"]
        seq_num.append(int(seq))
        
    idx = 0
    f = open("./send.json", "r", encoding = "utf-8")
    data = json.load(f)
    le = 0
    for item in data:
        if item['_source']['layers']['frame']['frame.time_epoch'] is None:
            continue
        
#        if 'data' not in item['_source']['layers'].keys():
#            continue
#        print(item['_source']['layers']['ip']['ip.src'])
        if 'ip' not in item['_source']['layers'].keys():
            continue
        if item['_source']['layers']['ip']['ip.src'] == "155.138.226.26" and item['_source']['layers']['ip']['ip.dst'] == "144.202.27.162":
            send_number += 1
        #hash_val = hash(item['_source']['layers']['data']['data.data'])
        seq = item['_source']['layers']["tcp"]["tcp.seq"]
        timestamp = eval(item['_source']['layers']['frame']['frame.time_epoch'])
        if int(seq) != seq_num[idx]:
            continue
        else:
            print("%.7lf"%abs(time_stamp[idx]-timestamp), file=fd)
            idx += 1
        if idx>=len(seq_num):
            break
    fd.close()
    