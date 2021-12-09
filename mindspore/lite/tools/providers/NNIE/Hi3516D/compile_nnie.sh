#!/bin/bash

# Build x86 for nnie
function Run_Build_x86() {
  # decompress release_pkg
  cd ${open_source_ms_path}/output/ || exit 1
  file_name=$(ls ./*linux-x64.tar.gz)
  IFS="-" read -r -a file_name_array <<< "$file_name"
  version=${file_name_array[2]}
  tar -xf mindspore-lite-${version}-linux-x64.tar.gz

  # cp tools folder
  cd ${open_source_ms_path}/output/mindspore-lite-${version}-linux-x64 || exit 1
  rm -rf ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/third_party/ms_lite/
  mkdir -p ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/third_party/ms_lite/ || exit 1
  cp -r ./tools/ ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/third_party/ms_lite/ || exit 1

  # compile nnie converter so
  export MSLITE_ENABLE_NNIE=on
  export MSLITE_ENABLE_CONVERTER=off
  export MSLITE_ENABLE_TRAIN=off
  export MSLITE_ENABLE_TOOLS=off
  export MSLITE_ENABLE_TESTCASES=off
  bash ${nnie_code_path}/mindspore/build.sh -I x86_64 -j ${thread_num}
  if [ $? = 0 ]; then
    echo "build x86 for nnie success"
    hi3516d_release_path=${open_source_ms_path}/output/mindspore-lite-${version}-linux-x64/tools/converter/providers/Hi3516D
    rm -rf ${hi3516d_release_path}
    mkdir -p ${hi3516d_release_path}/third_party/opencv-4.2.0
    mkdir -p ${hi3516d_release_path}/third_party/protobuf-3.9.0
    cp ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/providers/NNIE/Hi3516D/opencv-4.2.0/lib/* ${hi3516d_release_path}/third_party/opencv-4.2.0/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/providers/NNIE/Hi3516D/protobuf-3.9.0/lib/* ${hi3516d_release_path}/third_party/protobuf-3.9.0/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/providers/NNIE/Hi3516D/libnnie_mapper.so ${hi3516d_release_path}/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/build/tools/converter/adapter/nnie/libmslite_nnie_converter.so ${hi3516d_release_path}/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/build/tools/converter/adapter/nnie/data_process/libmslite_nnie_data_process.so ${hi3516d_release_path}/ || exit 1
    ms_config_file=${hi3516d_release_path}/../../converter/converter.cfg
    echo "[registry]" > ${ms_config_file}
    echo 'plugin_path=../providers/Hi3516D/libmslite_nnie_converter.so' >> ${ms_config_file}

    hi3559a_release_path=${open_source_ms_path}/output/mindspore-lite-${version}-linux-x64/tools/converter/providers/Hi3559A
    rm -rf ${hi3559a_release_path}
    mkdir -p ${hi3559a_release_path}
    cp ${nnie_code_path}/mindspore/mindspore/lite/tools/converter/adapter/nnie/providers/NNIE/Hi3559A/libnnie_mapper.so ${hi3559a_release_path}/ || exit 1

    cd ${open_source_ms_path}/output/ || exit 1
    # remove unused static library
    echo "cp new nnie so to release pkg success"
    rm ${open_source_ms_path}/output/mindspore-lite-${version}-linux-x64.tar.gz
    tar -zcf ./mindspore-lite-${version}-linux-x64.tar.gz ./mindspore-lite-${version}-linux-x64/ || exit 1
    sha256sum ./mindspore-lite-${version}-linux-x64.tar.gz > ./mindspore-lite-${version}-linux-x64.tar.gz.sha256 || exit 1
  else
    echo "build x86 for nnie failed"; return 1
  fi
}

# Build arm32 for nnie
function Run_Build_arm() {
  # decompress release_pkg
  cd ${open_source_ms_path}/output/ || exit 1
  file_name=$(ls ./*linux-${package_name}.tar.gz)
  IFS="-" read -r -a file_name_array <<< "$file_name"
  version=${file_name_array[2]}
  tar -xf mindspore-lite-${version}-linux-${package_name}.tar.gz

  # cp runtime folder
  cd ${open_source_ms_path}/output/mindspore-lite-${version}-linux-${package_name} || exit 1
  rm -rf ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie/third_patry/runtime/
  mkdir -p ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie/third_patry/runtime/ || exit 1
  rm -rf ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie_proposal/third_patry/runtime/
  mkdir -p ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie_proposal/third_patry/runtime/ || exit 1
  cp -r ./runtime/ ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie/third_patry/
  cp -r ./runtime/ ${nnie_code_path}/mindspore/mindspore/lite/tools/benchmark/nnie_proposal/third_patry/

  # compile nnie runtime so
  export TOOLCHAIN_NAME=${toolchain_name}
  export TOOLCHAIN_FILE=${open_source_ms_path}/mindspore/lite/cmake/${toolchain_name}.toolchain.cmake
  export MSLITE_REGISTRY_DEVICE=${device_name}

  # disable gpu & npu & train
  export MSLITE_GPU_BACKEND=off
  export MSLITE_ENABLE_NPU=off
  export MSLITE_ENABLE_TRAIN=off
  export MSLITE_ENABLE_NNIE=on

  bash ${nnie_code_path}/mindspore/build.sh -I ${task} -e cpu -j ${thread_num}
  if [ $? = 0 ]; then
    echo "build arm for nnie success"
    release_path=${open_source_ms_path}/output/mindspore-lite-${version}-linux-${package_name}/providers/${device_name}/
    rm -rf ${release_path}
    mkdir -p ${release_path}
    mkdir -p ${open_source_ms_path}/output/mindspore-lite-${version}-linux-${package_name}/tools/benchmark/
    cp ${nnie_code_path}/mindspore/mindspore/lite/build/tools/benchmark/benchmark ${open_source_ms_path}/output/mindspore-lite-${version}-linux-${package_name}/tools/benchmark/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/build/tools/benchmark/nnie/libmslite_nnie.so ${release_path}/ || exit 1
    cp ${nnie_code_path}/mindspore/mindspore/lite/build/tools/benchmark/nnie_proposal/libmslite_proposal.so ${release_path}/ || exit 1
    if [ ${device_name} == "Hi3516D" ]; then
      cp ${nnie_code_path}/mindspore/mindspore/lite/micro/example/hi3516d/libmicro_nnie.so ${release_path}/ || exit 1
    fi
    echo "cp new nnie so to release pkg success"
    cd ${open_source_ms_path}/output/ || exit 1
    rm ${open_source_ms_path}/output/mindspore-lite-${version}-linux-${package_name}.tar.gz
    tar -zcf ./mindspore-lite-${version}-linux-${package_name}.tar.gz ./mindspore-lite-${version}-linux-${package_name}/ || exit 1
    sha256sum ./mindspore-lite-${version}-linux-${package_name}.tar.gz > ./mindspore-lite-${version}-linux-${package_name}.tar.gz.sha256 || exit 1
  else
    echo "build arm for nnie failed"; return 1
  fi
}

# bashpath should be /home/jenkins/agent-working-dir/workspace/Compile_Lite_ARM32_3516D/
basepath=$(pwd)
echo "basepath is ${basepath}"
#set -e
open_source_ms_path=${basepath}/mindspore

# Example:sh compile_nnie.sh -I arm32 -b nnie_master
while getopts "I:b:j:t:d:" opt; do
    case ${opt} in
        I)
            task=${OPTARG}
            echo "compile task is ${OPTARG}"
            ;;
        b)
            branch_name=${OPTARG}
            echo "branch name is ${OPTARG}"
            ;;
        t)
            toolchain_name=${OPTARG}
            echo "toolchain_name is ${OPTARG}"
            ;;
        d)
            device_name=${OPTARG}
            echo "device_name is ${OPTARG}"
            ;;
        j)
            thread_num=${OPTARG}
            echo "thread_num is ${OPTARG}"
            ;;
        ?)
        echo "unknown para"
        exit 1;;
    esac
done

# download nnie repository
nnie_code_path=${basepath}/nnie_code/
rm -rf ${nnie_code_path}
mkdir -p ${nnie_code_path}
cd ${nnie_code_path} || exit 1

if [[ "${GITEE_CREDENTIAL_USR}" && "${GITEE_CREDENTIAL_PSW}" ]]; then
  gitee_user_name=${GITEE_CREDENTIAL_USR}
  gitee_user_pass=${GITEE_CREDENTIAL_PSW}
else
  echo "ERROR: ENV GITEE_CREDENTIAL_USR or GITEE_CREDENTIAL_PSW not found."
  exit 1
fi

git clone https://${gitee_user_name}:${gitee_user_pass}@gitee.com/ms-incubator/mindspore.git -b ${branch_name} --single-branch
if [[ $? -ne 0 ]]; then
  echo "git clone https://gitee.com/ms-incubator/mindspore.git -b ${branch_name} failed."
  exit 1
fi

if [ ${task} == "x86_64" ]; then
  echo "start building x86 for nnie..."
  Run_Build_x86
elif [ ${task} == "arm32" ]; then
  echo "start building arm32 for nnie..."
  package_name=aarch32
  Run_Build_arm
elif [ ${task} == "arm64" ]; then
  echo "start building arm64 for nnie..."
  package_name=aarch64
  Run_Build_arm
fi

Run_build_PID=$!
exit ${Run_build_PID}
