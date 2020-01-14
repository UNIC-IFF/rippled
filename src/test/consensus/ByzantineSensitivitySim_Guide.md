# Running Byzantine Sensitivity Tests

## Testing Scenarios

### Scenario SybilianTxSensitivitySim_ovUNL

### Scenario SybilianSensitivitySim_ovUNL

### Scenario ByzantineSensitivitySim_ovUNL

## How to run

```bash
./rippled --unittest ripple.consensus.ByzantineSensitivitySim --unitest-args "50 1 18 2 0.8 0.91 0.1"
```
The arguments order is : 
```
<Total Number of Validators> <Minimum number of malicious validators> < Maximum Number of malicious validators> <malicious validators step> < Minimum UNL overlapping ratio> < Maximum UNL overlapping ratio> < UNL overlapping step>  
```

## Expected output


## Arguments and Parameters of tests




## Contributors

Antonios Inglezakis (@ainglezakis) e-mail: inglezakis.a@unic.ac.cy

###### Draft Notes

Number of Nodes
Number of Malicious nodes
Transaction rate per injector/submitter
% of UNL overlapping
latency (fixed)
Simulation duration (minutes or seconds)

Scenarios:


ByzantineSensitivitySim

1 normalTxSubmitter( rate: 100tx/s, quiet_period: 10sec )
1 byzantineTxInjector ( rate: 100tx/s, quiet_period: 10sec)



SybilianSensitivitySim

1 normalTxSubmitter( rate: 100tx/s, quiet_period: 10sec )
1 SybilianTxInjector ( rate: 100tx/s. quiet_period: 10sec)



SybilianTxSensitivitySim

1 normalTxSubmitter( rate: 100tx/s, quiet_period: 10sec )
1 SybilianTxSubmitter (rate: 100tx/s, quiet_period: 10sec)