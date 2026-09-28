#!/bin/sh
for i in *TIDL*
do
    echo ${i}
    echo ${i##*MOCK_}
    git mv ${i} "ut_mock_${i##*MOCK_}"
done
