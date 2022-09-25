#include "../src/Lossdetection/full_flow_test_level.h"

using namespace std;
// TestFullFlow *test[10000000];


int main(int argc, char *argv[]) {
    int round = 10000;

    if (argc != 2) {
        cout << "./decode_test setting" << endl;
        return 0;
    }
    int flow = atoi(argv[1]);

    CDF_flows data(0);
    string output_file = "bucketPerFlow8-" + string(argv[1]) + ".xls";
    ofstream output(output_file);
    data.load_data_cnt("130000.dat", flow);

    cout << "dropped num: " << data.dropped_num << " packet num: " << data.packet_num << endl;

    output << "bucket\tfermat\tfermat(fp)\tfermat(fp8)" << endl;
    int acc_cnt1 = 0;
    int acc_cnt2 = 0;
    int acc_cnt3 = 0;
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    double dcd_per1 = 0;
    double dcd_per2 = 0;
    double dcd_per3 = 0;
    long long time1 = 0;
    long long time2 = 0;
    long long time3 = 0;
    int fing_win = 0;
    int fing_lose = 0;
    int grow = flow*0.001;
    if(grow == 0){
        grow = 1;
    }
    for (int bucket = flow*0.38; bucket < flow*0.6; bucket += grow) {
        bool t = true;
        acc_cnt1 = 0;
        acc_cnt2 = 0;
        acc_cnt3 = 0;
        dcd_per1 = 0;
        dcd_per2 = 0;
        dcd_per3 = 0;
        time1 = 0;
        time2 = 0;
        time3 = 0;
        retV r;
        for (int j = 0; j < round; j++) {
            TestFullFlow * test = new TestFullFlow(bucket * 3 * 8, rand() % 1000, 3);
            test->insert_flow(data);
            r = test->decode(data, output);
            if(r.flag1){
                acc_cnt1++;
                dcd_per1 += r.dcd_per1;
            }
            if(r.flag2){
                acc_cnt2++;
                dcd_per2 += r.dcd_per2;
            }
            if(r.flag3){
                acc_cnt3++;
                dcd_per3 += r.dcd_per3;
            }
            
            time1 += r.time1;
            time2 += r.time2;
            time3 += r.time3;
            if(!r.flag1 && (r.flag2 || r.flag3)) {
                fing_win++;
            }
            if(r.flag1 && !(r.flag2 && r.flag3)) {
                fing_lose++;
            }
            delete test;
        }
        output << bucket*3 << "\t" << dcd_per1/round << "\t" << dcd_per2/round << "\t" << dcd_per3/round << "\t" << "\t" << time1/round << "\t" << time2/round << "\t" << time3/round << endl;
        if (!flag1 && acc_cnt1 > 0.99*round) {
            flag1 = true;
            // output << "fermat_rehash\tbucket\t" << bucket*3 << "\tdecode_percent\t" << dcd_per1/round << "\ttime\t" << time1/round << endl;
        }
        if (!flag2 && acc_cnt2 > 0.99*round) {
            flag2 = true;
            // output << "fermat_fing1\tbucket\t" << bucket*3 << "\tdecode_percent\t" << dcd_per2/round << "\ttime\t" << time2/round << endl;
        }
        if (!flag3 && acc_cnt3 > 0.99*round) {
            flag3 = true;
            // output << "fermat_fing2\tbucket\t" << bucket*3 << "\tdecode_percent\t" << dcd_per3/round << "\ttime\t" << time3/round << endl;
        }
        if (flag1 && flag2 && flag3) {
            break;
        }
    }
    cout << "fing_win:" << fing_win << endl;
    cout << "fing_lose:" << fing_lose << endl;

    output.close();
    cout << "finish" << endl;

    return 0;
}
