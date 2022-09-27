#include "./src/Lossdetection/full_flow_test.h"

using namespace std;
// TestFullFlow *test[10000];


int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "./decode_test setting" << endl;
        return 0;
    }

    int thred = atoi(argv[3]);
    CDF_flows data(atoi(argv[2]));
    string output_file = "dropf-mem-" + string(argv[1]) + "_" + string(argv[2]) + "_" + string(argv[3]) + ".xls";
    ofstream output(output_file);
    data.load_data_drop("130000.dat", atoi(argv[1]));

    cout << "dropped num: " << data.dropped_num << " packet num: " << data.packet_num << endl;

    output << "memory\tfermat(fp)\tfermat\tlossradar\tflowradar" << endl;
    output << "dropped num\t" << data.dropped_num << "\tpacket num\t" << data.packet_num << endl;

    int cnt = 0;
    int mem = 0;
    for (mem = 155841; mem < 10000000; mem *= 1.1) {
        int cnt_c = 0;
        for(int i = 0; i < 1000; i += 1) {
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