The source codes and scripts of ChameleMon on edge switches.

### Codes

- `Hierarchical_Fermat.p4`: P4 source code of ChameleMon on switch data plane.
- `Hierarchical_Fermat.cpp`: CPP source code of ChameleMon on switch control plane.
- `tableinit.py`: Python source code of ChameleMon on switch control plane.

### Compile

`bash p4_build.sh Hierarchical_Fermat.p4`
`bash build.sh`

### Run the code

`bash run.sh [time_to_start_ChameleMon]`