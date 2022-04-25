#!/bin/bash

shabu_in="shabu"
shabu_out="out"
exe=$shabu_out/shabu

in_dir=".."
out_dir="../vulkify/src/spir_v"
shaders=("test.vert" "test.frag")
names=("test_vert_v" "test_frag_v")
files=("vert.spv.hpp" "frag.spv.hpp")

cd "$(dirname "${BASH_SOURCE[0]}")"

cf_available=false
if [[ $(clang-format --version) ]]; then
  cf_available=true
fi

if [[ ! $(glslc --version) ]]; then
  echo -e "glslc not found\n"
  exit 1
fi

if [[ ! -f $shabu_in/CMakeLists.txt ]]; then
  echo -e "$shabu_in/CMakeLists.txt not found"
  exit 1
fi

if [[ ! -d $shabu_out/CMakeCache.txt ]]; then
  cmake -S $shabu_in -B $shabu_out -DCMAKE_BUILD_TYPE=Release
  if [[ ! $? ]]; then
    echo -e "failed to configure shabu\n"
    exit 1
  fi
fi

cmake --build $shabu_out
if [[ ! $? ]]; then
  echo -e "failed to build shabu\n"
  exit 1
fi

for i in ${!shaders[@]}; do
  shader=${shaders[$i]}
  name=${names[$i]}
  file=${files[$i]}
  
  glslc $in_dir/$shader -o $in_dir/$shader.spv
  if [[ ! $? ]]; then
    echo -e "failed to compile [$in_dir/$shader] to [$in_dir/$shader.spv]\n"
    exit 1
  fi
  echo "== compiled [$in_dir/$shader] to [$in_dir/$shader.spv]"

  $exe $in_dir/$shader.spv $name > $out_dir/$file
  if [[ ! $? ]]; then
    echo -e "failed to burn [$shader.spv] to [$out_dir/$file]\n"
    exit 1
  fi
  if [[ $cf_available == true ]]; then
    clang-format $out_dir/$file -i $out_dir/$file
  fi
  echo "== burnt [$shader.spv] to [$out_dir/$file]"
done

exit
