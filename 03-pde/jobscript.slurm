#!/bin/bash
#SBATCH --time=1

# Starte print_output.sh on 5 Knots with 5 Processes
#SBATCH --nodes=5
#SBATCH --ntasks-per-node=5
#SBATCH --ntasks=25

# Setze feste Partition (west)
#SBATCH --partition=west

# All output into print_output.out
#SBATCH --output=print_output.out

srun ./print_output.sh
echo "Fertig" > jobscript.out
