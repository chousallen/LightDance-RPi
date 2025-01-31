#include <fcntl.h>  // O_WRONLY
#include <stdio.h>
#include <stdlib.h>  // exit
#include <sys/stat.h>
#include <time.h>  // time
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <string>

#include "command.h"

using namespace std;
#define secAdjust 1000000
#define msecAdjust 1000
class Play : public Command {
   public:
    Play() : Command() {
        addFlag("-h", "help");
        addFlag("-t", "ms");
    }
    int execute(int argc, char* argv[]) {
        if (cmdOptionExists(argv, argv + argc, "-h")) {
            help();
            return 0;
        }
        if (argc == 1) {
            cout << "Missing command.\n";
            cout << "It should be either \"play\" or \"pause\" or \"stop\" or "
                    "\"restart\" or \"quit\".\n";
            cout << "Please use \"-h\" for help." << endl;
            return 1;
        }
        if (argc >= 2) {
            string cmd = argv[1];
            transform(cmd.begin(), cmd.end(), cmd.begin(),
                      [](unsigned char c) { return tolower(c); });
            if (set<string>{"pause", "quit", "restart", "stop"}.count(cmd) >
                0) {
                // test
                cout << "sendToPlayLoop(cmd), cmd = " << cmd << endl;
                sendToPlayLoop(cmd);
            }
            if (cmd.compare("play") == 0) {
                vector<int> delayTime;  // ms
                bool sExist = cmdOptionExists(argv, argv + argc, "-s");
                bool secExist = cmdOptionExists(argv, argv + argc, "--sec");
                bool dExist = cmdOptionExists(argv, argv + argc, "-d");
                bool delayExist = cmdOptionExists(argv, argv + argc, "--delay");
                bool needDelay = dExist || delayExist;

                // get delayTime
                if (needDelay) {
                    delayTime = getCmdOptionInt(
                        argv, argv + argc,
                        dExist ? "-d" : "--delay");  // default ms
                    // test
                    cout << "delayTime = ";
                    for (const auto& i : delayTime) cout << i << ' ';
                    cout << endl;
                    string delayFlag = dExist ? "-d" : "--delay";

                    if (delayTime.size() == 0) {
                        cerr << "Error in " << delayFlag << " option.";
                        cerr << "It should have <time> after " << delayFlag
                             << " flag." << endl;
                        return 1;
                    }
                }

                // get params for play()
                vector<long> time;
                vector<float> timeSec;
                vector<long> timeMsec;
                string secFlag;

                if (sExist || secExist) {
                    timeSec = getCmdOptionFloat(argv, argv + argc,
                                                sExist ? "-s" : "--sec");
                    time.resize(timeSec.size());
                    for (int i = 0; i < time.size(); i++)
                        time[i] = timeSec[i] * secAdjust;
                } else {
                    timeMsec = getCmdOptionLong(argv, argv + argc, "play");
                    time.resize(timeMsec.size());
                    for (int i = 0; i < time.size(); i++)
                        time[i] = timeMsec[i] * msecAdjust;
                }

                // test
                cout << "time = ";
                for (const auto& i : time) cout << i << ' ';
                cout << endl;

                // check time range
                for (const auto& i : time) {
                    if (i < -1.0) {
                        cerr << "It might be overflowed.\nPlease check "
                                "your input!."
                             << endl;
                        return 1;
                    }
                }

                // call play()
                secFlag = (sExist ? "s" : (secExist ? "sec" : "play"));

                if (time.size() == 0) {
                    if (secFlag == "play") {
                        // test
                        cout << "play, time.size() = 0" << endl;
                        return play(0, -1, needDelay, delayTime);
                    } else {
                        cerr << "Error in " << secFlag << " option." << '\t';
                        cerr << "It should have <time> after " << secFlag
                             << " flag." << endl;
                        return 1;
                    }
                } else if (time.size() == 1) {
                    // test
                    cout << "play, time.size() = 1" << endl;
                    return play(time[0], -1, needDelay, delayTime);
                } else if (time.size() == 2) {
                    // test
                    cout << "play, time.size() = 2" << endl;
                    return play(time[0], time[1], needDelay, delayTime);
                } else {
                    cerr << "Error in " << secFlag
                         << " option.\nPlease use \"-h\" for "
                            "help."
                         << endl;

                    return 1;
                }
            }
        }
        return 0;
    }

   private:
    int play(long start, long end, bool needDelay, vector<int> delayTime) {
        stringstream cmd;
        // cout << delayTime[0] << endl;
        cmd << "play " << start << " " << end << " -d "
            << (needDelay ? ((long)delayTime[0] * msecAdjust) : 0);

        string mycmd = cmd.str();
        sendToPlayLoop(mycmd);
        cout << mycmd << endl;
        return 0;
    }
    int sendToPlayLoop(string msg) {
        int fd;
        int n;
        char buf[1024];
        mkfifo("/tmp/cmd_to_player", 0666);
        if ((fd = open("/tmp/cmd_to_player", O_WRONLY)) < 0) {
            perror("Open FIFO Failed");
            return 1;
        }

        n = sprintf(buf, "%s", msg.c_str());

        // printf("Send message: %s\n", buf);
        if (write(fd, buf, n + 1) < 0) {
            perror("Write FIFO Failed");
            close(fd);
            return 1;
        }
        int res_fd;
        char res_buf[1024];

        string resFIFOpath = "/tmp/player_to_cmd";
        if ((res_fd = open(resFIFOpath.c_str(), O_RDONLY)) < 0) {
            perror("Open response FIFO Failed");
            return 1;
        }

        read(res_fd, res_buf, 1024);
        if (res_buf[0] == '0')
            printf("Success!\n");
        else
            printf("Failed!\n");
        close(fd);  // should be comment in formal version
        return 0;
    }
};

int main(int argc, char* argv[]) {
    Play cmd;
    return cmd.execute(argc, argv);
}
