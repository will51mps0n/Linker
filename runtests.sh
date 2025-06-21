#!/usr/bin/env bash
make clean
make linker

declare -a tests=(
  "main.obj subone.obj count5.mc count5.mc.correct"
  "t1_0.obj t1.mc t1c.txt"
  "t3_0.obj t3_1.obj t3.mc t3c.txt"
  "t4_0.obj t4_1.obj t4.mc t4c.txt"
  "t5_0.obj t5_1.obj t5_2.obj t5.mc t5c.txt"
  "t6_0.obj t6.mc t6c.txt"
  "t7_0.obj t7_1.obj t7_2.obj t7.mc t7c.txt"
  "t8_0.obj t8_1.obj t8_2.obj t8.mc t8c.txt"
  "t9_0.obj t9_1.obj t9_2.obj t9.mc t9c.txt"
  "t10_0.obj t10_1.obj t10_2.obj t10.mc t10c.txt"
  "t11_0.obj t11_1.obj t11.mc" # no .txt comparison
  "t2_0.obj t2_1.obj t2_2.obj t2.mc t2c.txt"
)

cd Linker_Tests

for entry in "${tests[@]}"; do
  args=($entry)
  obj_files=("${args[@]:0:${#args[@]}-2}")
  outfile="${args[-2]}"
  correct="${args[-1]}"

  echo "Running: ./linker ${obj_files[*]} $outfile"
  ../../linker "${obj_files[@]}" "$outfile"

  if [[ -f "$correct" ]]; then
    echo "Diff vs $correct:"
    diff "$outfile" "$correct"
    echo
  fi
done