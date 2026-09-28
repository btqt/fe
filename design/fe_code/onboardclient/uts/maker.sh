
ROOT_PATH=${PWD}

unset files
unset module

ut_dir=($(find ${ROOT_PATH} -name 'unittests' -type d))
files=($(find ${ut_dir} -name 'UT_*.cpp' -type f))
echo "${ut_dir}"

PROJECT="${ROOT_PATH%"${ROOT_PATH##*[!/]}"}" 
PROJECT=${PROJECT##*/}
echo "#$PROJECT#";

JOBS=32

i=0
for item in ${files[*]}
do
    temp=$(basename "$item")

    module[i]=${temp:9:-4}
    i=$((i+1))
done
module[i]="all"

readarray -t module < <(printf '%s\0' "${module[@]}" | sort -z | xargs -0n1)

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     MACHINE=LINUX;;
    CYGWIN*)    MACHINE=CYGWIN;;
    MINGW*)     MACHINE=MINGW;;
    *)          MACHINE="UNKNOWN:${unameOut}"
esac
echo ${MACHINE}

function uts_help() {
cat <<EOF
Invoke ". ./maker.sh" from your shell to add the following functions to your environment:
    1. help :
    2. build :
    3. run :
    4. exit :
EOF
}

function uts_make() {
    read -p "Will you generate unittests again (y/n)? " answer
    case ${answer:0:1} in
        y|Y )
            echo Yes;;
        * )
            return
    esac

    if [  -d "unittests"  ]; then
        if [ ! -d "backup"  ]; then
            mkdir backup
        fi
        mv unittests* ./backup/
    fi

    ./Maker.py --all --enable_tc 2>&1 | tee -a all_maker.log

    cd unittests
    if [ ! -d "build"  ]; then
        mkdir build
    fi
    cd build
    cmake ..

    cd ${ROOT_PATH}

    uts_help
}

function uts_build() {
    select opt1 in "${module[@]}"; do 
        case "$REPLY" in
        ? ) 
            MODULE=${opt1}
            break;;
        1? ) 
            MODULE=${opt1}
            break;;
        2? ) 
            MODULE=${opt1}
            break;;
        3? ) 
            MODULE=${opt1}
            break;;
        4? ) 
            MODULE=${opt1}
            break;;
        esac
    done

    cd unittests
    if [  -d "build"  ]; then
        rm -rf ./build
    fi
    mkdir build
    cd build
    cmake ..

    if [  -f dbg.txt  ]; then
        echo "Remove dbg.txt!"
        rm dbg.txt
    fi

    if [  -f error.txt  ]; then
        echo "Remove error.txt!"
        rm error.txt
    fi
    make ${MODULE} VERBOSE=1 -j${JOBS} 2>&1 | tee dbg.txt
    cat dbg.txt 2>&1 | grep error >> error.txt

    vi error.txt
    vi dbg.txt

    cd ${ROOT_PATH}
    uts_help
}

function uts_run() {
    select opt1 in "${module[@]}"; do 
        case "$REPLY" in
        ? ) 
            MODULE=${opt1}
            break;;
        1? ) 
            MODULE=${opt1}
            break;;
        2? ) 
            MODULE=${opt1}
            break;;
        3? ) 
            MODULE=${opt1}
            break;;
        4? ) 
            MODULE=${opt1}
            break;;
        esac
    done

    cd unittests
    if [  -d "build"  ]; then
        rm -rf build
    fi
    mkdir build
    cd build
    cmake ..

    if [  -f dbg_run.txt  ]; then
        echo "Remove dbg_run.txt!"
        rm dbg_run.txt
    fi

    if [  -d "${MODULE}_coverage"  ]; then
        echo "Remove rm -rf ${MODULE}_coverage"
        rm -rf ${MODULE}_coverage
    fi

    if [ ! -f run-coverage_arm.sh  ]; then
        cp ${ROOT_PATH}/run-coverage_arm.sh ./
    fi

    make ${MODULE} VERBOSE=1 -j${JOBS} 2>&1 | tee -a dbg_run.txt

    if [ "$MODULE" = "all"  ]; then
        echo "make ${MODULE}_coverage -j${JOBS} 2>&1 | tee -a dbg_run.txt"
        make all_coverage -j${JOBS} 2>&1 | tee -a dbg_run.txt
    else
        echo "make ${MODULE}_coverage -j${JOBS} 2>&1 | tee -a dbg_run.txt"
        make ${MODULE}_coverage -j${JOBS} 2>&1 | tee -a dbg_run.txt
    fi

    # cat dbg_run.txt 2>&1 | grep error

    #vi dbg_run.txt

    cd ${ROOT_PATH}
    uts_help
}

function uts_doxygen() {
    echo "Generate doxygen document"
    cd ${ROOT_PATH}
    if [ ! -d "doxygen"  ]; then
        mkdir doxygen
    fi
    cd ./doxygen
    doxygen ../Doxyfile
    cd ${ROOT_PATH}
}

function uts_debug() {
    echo "test"
}

OPTIONS="help build run exit"
select opt in $OPTIONS; do
    if [ "$opt" = "help" ]; then
        uts_help
    elif [ "$opt" = "build" ]; then
        uts_build
    elif [ "$opt" = "run" ]; then
        uts_run
    elif [ "$opt" = "exit" ]; then
        break
    else
     clear
     echo "bad option"
    fi
done
