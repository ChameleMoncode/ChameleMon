#include "../src/CMSketch/CM.h"
#include "../src/CUSketch/CU.h"
#include "../src/Fermat/Fermat_tower.h"
#include "../src/ElasticP4/our_elastic.h"
#include "../src/FCMelastic/FCMelastic.h"
#include <iomanip>
int main()
{
    uint32_t totnum_packet = ReadTraces();

    
    Fermat_tower *fermat = NULL;
    CMSketch *cm = NULL;
    CUSketch<4, CU_DEPTH> *cu = NULL;
    ElasticSketch *elastic = NULL;
    FCMPlus *fcmplus = NULL;
    unordered_map<uint32_t, uint32_t> true_freq;
    double Fermat_small_ARE = 0;
    double Fermat_mid_ARE = 0;
    double Fermat_big_ARE = 0;
    double CM_small_ARE = 0;
    double CM_mid_ARE = 0;
    double CM_big_ARE = 0;
    double CU_small_ARE = 0;
    double CU_mid_ARE = 0;
    double CU_big_ARE = 0;
    double Elastic_small_ARE = 0;
    double Elastic_mid_ARE = 0;
    double Elastic_big_ARE = 0;
    double FCM_small_ARE = 0;
    double FCM_mid_ARE = 0;
    double FCM_big_ARE = 0;
    int num_pkt = (int)traces[0].size();
    for (int i = 0; i < num_pkt; ++i)
    {
        true_freq[*((uint32_t *)(traces[0][i].key))]++;
    }
    unordered_map<uint32_t, uint32_t> small_flows, mid_flows, big_flows;
    for (unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
    {
        if (it->second <= 50)
            small_flows.insert(*it);
        else if(it->second <= 250)
            mid_flows.insert(*it);
        else
            big_flows.insert(*it);
    }
    for (int times = 0; times < TIMES; times++)
    {
        fermat = new Fermat_tower();
        cm = new CMSketch(CM_BYTES);
        cu = new CUSketch<4, CU_DEPTH>(CU_BYTES);
        elastic = new ElasticSketch();
        fcmplus = new FCMPlus();

        for (int i = 0; i < num_pkt; ++i)
        {
            fermat->insert((const char *)(traces[0][i].key), 1);
            cm->insert((uint8_t *)(traces[0][i].key), 1);
            cu->insert((uint8_t *)(traces[0][i].key), 1);
            elastic->insert((uint8_t *)(traces[0][i].key), 1);
            fcmplus->insert((uint8_t *)(traces[0][i].key), 1);
        }
        fermat->decode();
        /*-*-*-* End of packet insertion *-*-*-*/
        double Fermat_small_are = 0;
        double Fermat_mid_are = 0;
        double Fermat_big_are = 0;
        double CM_small_are = 0;
        double CM_mid_are = 0;
        double CM_big_are = 0;
        double CU_small_are = 0;
        double CU_mid_are = 0;
        double CU_big_are = 0;
        double Elastic_small_are = 0;
        double Elastic_mid_are = 0;
        double Elastic_big_are = 0;
        double FCM_small_are = 0;
        double FCM_mid_are = 0;
        double FCM_big_are = 0;
        int est_val;
        int dist;
        for(auto f : small_flows)
        {
            est_val = (int)fermat->query((const char*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Fermat_small_are += dist * 1.0 / f.second;
            est_val = (int)cm->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CM_small_are += dist * 1.0 / f.second;
            est_val = (int)cu->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CU_small_are += dist * 1.0 / f.second;
            est_val = (int)elastic->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Elastic_small_are += dist * 1.0 / f.second;
            est_val = (int)fcmplus->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            FCM_small_are += dist * 1.0 / f.second;
        }
        for(auto f : mid_flows)
        {
            est_val = (int)fermat->query((const char*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Fermat_mid_are += dist * 1.0 / f.second;
            est_val = (int)cm->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CM_mid_are += dist * 1.0 / f.second;
            est_val = (int)cu->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CU_mid_are += dist * 1.0 / f.second;
            est_val = (int)elastic->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Elastic_mid_are += dist * 1.0 / f.second;
            est_val = (int)fcmplus->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            FCM_mid_are += dist * 1.0 / f.second;
        }
        for(auto f : big_flows)
        {
            est_val = (int)fermat->query((const char*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Fermat_big_are += dist * 1.0 / f.second;
            est_val = (int)cm->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CM_big_are += dist * 1.0 / f.second;
            est_val = (int)cu->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            CU_big_are += dist * 1.0 / f.second;
            est_val = (int)elastic->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            Elastic_big_are += dist * 1.0 / f.second;
            est_val = (int)fcmplus->query((uint8_t*)&f.first);
            dist = abs((int)f.second - (int)est_val);
            FCM_big_are += dist * 1.0 / f.second;
        }
        Fermat_small_ARE += Fermat_small_are / small_flows.size();
        CM_small_ARE += CM_small_are / small_flows.size();
        CU_small_ARE += CU_small_are / small_flows.size();
        Elastic_small_ARE += Elastic_small_are / small_flows.size();
        FCM_small_ARE += FCM_small_are / small_flows.size();

        Fermat_mid_ARE += Fermat_mid_are / mid_flows.size();
        CM_mid_ARE += CM_mid_are / mid_flows.size();
        CU_mid_ARE += CU_mid_are / mid_flows.size();
        Elastic_mid_ARE += Elastic_mid_are / mid_flows.size();
        FCM_mid_ARE += FCM_mid_are / mid_flows.size();

        Fermat_big_ARE += Fermat_big_are / big_flows.size();
        CM_big_ARE += CM_big_are / big_flows.size();
        CU_big_ARE += CU_big_are / big_flows.size();
        Elastic_big_ARE += Elastic_big_are / big_flows.size();
        FCM_big_ARE += FCM_big_are / big_flows.size();
        delete fermat;
        delete cm;
        delete cu;
        delete elastic;
        delete fcmplus;
    }
    Fermat_small_ARE /= TIMES;
    CM_small_ARE /= TIMES;
    CU_small_ARE /= TIMES;
    Elastic_small_ARE /= TIMES;
    FCM_small_ARE /= TIMES;
    Fermat_mid_ARE /= TIMES;
    CM_mid_ARE /= TIMES;
    CU_mid_ARE /= TIMES;
    Elastic_mid_ARE /= TIMES;
    FCM_mid_ARE /= TIMES;
    Fermat_big_ARE /= TIMES;
    CM_big_ARE /= TIMES;
    CU_big_ARE /= TIMES;
    Elastic_big_ARE /= TIMES;
    FCM_big_ARE /= TIMES;
    cout << fixed << setprecision(8);
    cout << "Fermat_small_ARE : " << Fermat_small_ARE << endl;
    cout << "CM_small_ARE : " << CM_small_ARE <<endl;
    cout << "CU_small_ARE : " << CU_small_ARE <<endl;
    cout << "Elastic_small_ARE : " << Elastic_small_ARE << endl;
    cout << "FCM_small_ARE : " << FCM_small_ARE << endl;
    cout << "Fermat_mid_ARE : " << Fermat_mid_ARE << endl;
    cout << "CM_mid_ARE : " << CM_mid_ARE <<endl;
    cout << "CU_mid_ARE : " << CU_mid_ARE <<endl;
    cout << "Elastic_mid_ARE : " << Elastic_mid_ARE << endl;
    cout << "FCM_mid_ARE : " << FCM_mid_ARE << endl;
    cout << "Fermat_big_ARE : " << Fermat_big_ARE << endl;
    cout << "CM_big_ARE : " << CM_big_ARE <<endl;
    cout << "CU_big_ARE : " << CU_big_ARE <<endl;
    cout << "Elastic_big_ARE : " << Elastic_big_ARE << endl;
    cout << "FCM_big_ARE : " << FCM_big_ARE << endl;
}