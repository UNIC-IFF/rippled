//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2017 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <test/csf.h>
#include <utility>
#include <random>

namespace ripple {
namespace test {

class ByzantineSensitivitySim_test : public beast::unit_test::suite
{

    /**
         * Plan for this simulator
         * 1) Read the parameters from a file OR IDEALY from the command line arguments
         * 2) Create a group of non-malicious validators
         * 3) Create a group of malicious validators
         * 4) Form the common (overlapping) UNL between all the nodes, within the limits of UNL list size
         *      In order to do so, we randomly choose the (1-malpC) * #total_validators from non-malicious set
         *      and the rest malpC * #total_validators from the malicious set
         * 5) randomly select the rest validators in each UNL in respect to the malicous percentage
         * 6) Register the connections to the peerGroups
         * 7) Create Tx txSubmitter()
         * 8) 
        */
    void
    ByzantineSensitivitySim_ovUNL(
        std::size_t numPeers,
        std::size_t numByzantines,
        float overlapping_factor,
        csf::SimDuration delay = std::chrono::milliseconds(200),
        bool printHeaders = false)
    {//std::random_device myrng=std::default_random_engine{2})
        using namespace csf;
        using namespace std::chrono;

        // Initialize persistent collector logs specific to this method
        std::string const prefix =
                "ByzantineSensitivity__"
                "varyingUNLoverlapping";
        std::fstream
                txLog(prefix + "_tx.csv", std::ofstream::app),
                ledgerLog(prefix + "_ledger.csv", std::ofstream::app);

        // title
        log << prefix << "(" << numPeers << ","<< numByzantines << ","<< overlapping_factor << "," << delay.count() << ")"
            << std::endl;

        // number of peers, UNLs, connections
        int const numCNLs    = std::max(int(1.00 * numPeers), 1);
        int const maxCNLSize = std::max(int(0.50 * numCNLs),  1);
        int const minCNLSize = std::max(int(overlapping_factor * maxCNLSize),  std::max(int(0.25*numCNLs),1));
        int const commonUNLSize = overlapping_factor*maxCNLSize;

        log << "test1"<<std::endl;

        BEAST_EXPECT(numPeers >= 1);
        BEAST_EXPECT(numCNLs >= 1);
        log << "test1"<<std::endl;
        BEAST_EXPECT(1 <= minCNLSize
                && minCNLSize <= maxCNLSize
                && minCNLSize >= commonUNLSize
                && maxCNLSize <= numPeers);
        log << "test1"<<std::endl;

        Sim sim;
        
        PeerGroup normalValidators= sim.createGroup(numPeers-numByzantines);
        PeerGroup byzantines;
        PeerGroup network;

        if (numByzantines>0)
        {
            byzantines= sim.createGroup(numByzantines);
            network = normalValidators + byzantines;
        }
        else
        {
            network= normalValidators;
        }
        

        log << "test2"<<std::endl;

        // form common UNL
        PeerGroup commonUNL;

        // Create Normal Peer Selector
        std::vector<double> const normalRanks=
                sample(normalValidators.size(), PowerLawDistribution{1,3}, sim.rng);
        auto normalPeerSelector=
                makeSelector(normalValidators.begin(),
                                normalValidators.end(),
                                normalRanks, sim.rng);

        for (int i=0; i<commonUNLSize*(1-(numByzantines/numPeers));i++)
        {
            commonUNL=commonUNL + normalPeerSelector();
        }
        
        
        // Create Random Byzantine Peer Selector
        std::vector<double> const byzantineRanks=
                sample(byzantines.size(), PowerLawDistribution{1,3}, sim.rng);
        
        
        auto byzantinePeerSelector=
                makeSelector(byzantines.begin(),
                                byzantines.end(),
                                byzantineRanks, sim.rng);
        
        for (int i=0; i<commonUNLSize*(numByzantines/numPeers);i++)
        {
            commonUNL=commonUNL + byzantinePeerSelector();
        }
        
        

        // Create Random Peer Selector from all the network
        std::vector<double> const peerRanks=
                sample(network.size(), PowerLawDistribution{1,3}, sim.rng);
        auto networkPeerSelector=
                makeSelector(network.begin(), network.end(), peerRanks, sim.rng);

        // auto rng = std::default_random_engine{2};
        // std::shuffle(std::begin(mvector),std::end(mvector),rng);

        // std::vector<int> const UNLsizes=
        //         sample(network.size(), std::uniform_int_distribution{minCNLSize,maxCNLSize},sim.rng);
        std::default_random_engine myrng;
        std::uniform_int_distribution mydist (minCNLSize-commonUNLSize,maxCNLSize-commonUNLSize);

        for (auto ptr=network.begin();ptr!=network.end();advance(ptr,1))
        {
            // get a random number for UNL size of the peer.
            int extrapeers= mydist(myrng);
            int normalExtrapeers=(1-(numByzantines/numPeers))*extrapeers;
            int addedpeers=0;
            PeerGroup tmpUNL=commonUNL;
            PeerGroup tmpGroup= PeerGroup(*ptr);
            // ptr->connectandtrust(commonUNL);
            // ptr->connect(commonUNL);
            // ptr->trust(commonUNL);
            while (addedpeers<normalExtrapeers)
            {
                auto rp=normalPeerSelector();
                // check if in commonUNL
                
                if (tmpUNL.contains(rp))
                {
                    continue;
                }
                // ptr->connectandtrust(PeerGroup(rp));
                tmpUNL=tmpUNL+PeerGroup(rp);
                addedpeers++;
            }

            while (addedpeers<extrapeers)
            {
                auto rp=byzantinePeerSelector();
                // check if in commonUNL
                
                if (tmpUNL.contains(rp))
                {
                    continue;
                }
                // ptr->connectandtrust(PeerGroup(rp));
                tmpUNL=tmpUNL+PeerGroup(rp);
                addedpeers++;
            }
            
            tmpGroup.trustAndConnect(tmpUNL,delay);
        }

        // Initialize the data collectors
        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

        sim.run(1);


        // Run for 10 minues, submitting 100 tx/second
        std::chrono::nanoseconds const simDuration = 2min;//10min;
        std::chrono::nanoseconds const quiet = 10s;
        Rate const rate{100, 1000ms};

        // Initialize timers
        HeartbeatTimer heart(sim.scheduler);

        // txs, start/stop/step, target
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                                     sim.scheduler.now() + quiet,
                                     sim.scheduler.now() + simDuration - quiet,
                                     normalPeerSelector,
                                     sim.scheduler,
                                     sim.rng);
        // auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
        //                              sim.scheduler.now() + quiet,
        //                              sim.scheduler.now() + simDuration - quiet,
        //                              normalPeerSelector,
        //                              sim.scheduler,
        //                              sim.rng);
        auto txInjector = makeInjector(ConstantDistribution{rate.inv()},
                                    sim.scheduler.now()+ quiet,
                                    sim.scheduler.now()+ simDuration -quiet,
                                    byzantinePeerSelector,
                                    sim.scheduler,
                                    sim.rng);

        // run simulation for given duration
        heart.start();
        sim.run(simDuration);

        log << std::right;
        log << "| Peers: "<< std::setw(2) << numPeers;
        log << " | Byzantines: "<< std::setw(2) << numByzantines ;
        log << " | UNL overlapping: " << std::setw(2) << overlapping_factor << std::endl;
        log << "| Duration: " << std::setw(2)
            << duration_cast<milliseconds>(simDuration).count() << " ms";
        log << " | Branches: " << std::setw(1) << sim.branches();
        log << " | Synchronized: " << std::setw(1)
            << (sim.synchronized() ? "Y" : "N");
        log << " |" << std::endl;

        
        txCollector.report(simDuration, log, true);
        ledgerCollector.report(simDuration, log, false);

        std::string const tag = "\"( "+ std::to_string(numPeers) + 
                            ","+std::to_string(numByzantines)+ ","+
                             std::to_string(overlapping_factor) +")\"";

        txCollector.csv(simDuration, txLog, tag, printHeaders);
        ledgerCollector.csv(simDuration, ledgerLog, tag, printHeaders);

        log << std::endl;
    }

    void
    run() override
    {
        using namespace csf;
        using namespace std::chrono;

        // This test simulates a specific topology with nodes generating
        // different ledgers due to a simulated byzantine failure (injecting
        // an extra non-consensus transaction).

        Sim sim;
        ConsensusParms const parms{};

        SimDuration const delay =
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
        


        std::string const defaultArgs = "50 0 18 2 0.1 0.9 0.1";
        std::string const args = arg().empty() ? defaultArgs : arg();
        std::stringstream argStream(args);

        int totalNumValidators = 0;
        // int delayCount(200);
        int mintotalByzantines = 0,maxtotalByzantines = 0, byzantines_step=2;
        float minUNLoverlappingRatio= 0.1 , maxUNLoverlappingRatio= 0.9, ovUNL_step=0.1;
        argStream >> totalNumValidators;
        argStream >> mintotalByzantines;
        argStream >> maxtotalByzantines;
        argStream >> byzantines_step;
        argStream >> minUNLoverlappingRatio;
        argStream >> maxUNLoverlappingRatio;
        argStream >> ovUNL_step;

        // std::chrono::milliseconds const delay(delayCount);

        log << "ByzantineSensitivitySim: " << totalNumValidators << " Peers" << std::endl;
        log << std::setw(4) << "Min Byzantines: " << mintotalByzantines << " Max Byzantines: " << maxtotalByzantines << " sim step: " << byzantines_step << std::endl;
        log << std::setw(4) << "Min UNL overlapping: " << minUNLoverlappingRatio << " Max UNL overlapping: " << maxUNLoverlappingRatio << "sim step: "<< ovUNL_step << std::endl;
        
        bool printHeaders=true;
        for (int bi=mintotalByzantines; bi<=maxtotalByzantines; bi+=byzantines_step)
        {
            for (float ovUNL=minUNLoverlappingRatio; ovUNL<=maxUNLoverlappingRatio; ovUNL+=ovUNL_step)
            {
                log << "Starting..." << std::endl;
                ByzantineSensitivitySim_ovUNL(totalNumValidators,bi,ovUNL,delay, printHeaders);
                printHeaders=false;
            }
        }
        // ByzantineSensitivitySim_ovUNL(totalNumValidators,totalByzantines,UNLoverlappingRatio,delay, true);

    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(ByzantineSensitivitySim, consensus, ripple);

}  // namespace test
}  // namespace ripple
