#!/bin/bash

qsub -I <<EOF
#!/bin/bash
echo test

EOF
