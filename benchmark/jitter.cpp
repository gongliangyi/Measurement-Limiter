#include <bits/stdc++.h>
#include <sstream>
using namespace std;


int main()
{
    freopen("iperf_send.txt", "r", stdin);
    freopen("jitter.txt", "w", stdout);
    string line;
    int id_number = 0;
    while(getline(cin, line)){
        stringstream ss(line);
        int cnt = 0;
        int ans;
        string now;
        bool flag = false;
        double jitter = 0;
        while(ss>>now){
            cnt++;
            if(now == "ID]"){
                id_number++;
                if(id_number == 2){
                    break;
                }
            }
            if(id_number == 2 && cnt == 8){
                ss>>jitter;
                printf("%.3lf\n", jitter);
            }
        }
    }
    return 0;
}
