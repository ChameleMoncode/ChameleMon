#include "../src/Lossdetection/full_flow_test.h"

using namespace std;


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "./decode_test setting" << endl;
        return 0;
    }

    int thred = atoi(argv[2]);
    CDF_flows data(atoi(argv[1]));
    string output_file = "flow-mem-" + string(argv[1]) + "_" + string(argv[2]) + ".xls";
    ofstream output(output_file);
    data.load_data_flow("130000.dat", atoi(argv[1]));

    cout << "dropped num: " << data.dropped_num << " packet num: " << data.packet_num << endl;

    output << "memory\tfermat(fp)\tfermat\tlossradar\tflowradar" << endl;
    output << "dropped num\t" << data.dropped_num << "\tpacket num\t" << data.packet_num << endl;

    // TestFullFlow *test[10000];
    int cnt = 0;
    int mem = 0;
    for (mem = 17409; mem < 10000000; mem *= 1.1) {
        int cnt_c = 0;
        for(int i = 0; i < 100; i += 1) {
            TestFullFlow * test = new TestFullFlow(mem, i);
            test->insert_flow(data);
            bool t = test->decode(data, output);
            // delete test[cnt];
            // cnt++;
            if(t) {
                cnt_c++;
            }
        }
        if(cnt_c >= thred)
            break;
    }


    output.close();
    cout << mem << endl;
    cout << "finish" << endl;

    return 0;
}