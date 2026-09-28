
MYCODE=".."
TEMPLATE="_TIDL_origin/Code_Generator/OUTPUT/stc"
MODULENAME=`cat modulename`
REVISION=`cat revision`
DN_REVISION="master"


all: merge_and_compare

merge_and_compare: auto_merge merge_status

auto_merge:
	make gen
	cd ${MYCODE}
	@echo "###"
	@echo "### copy csv file : tidl.csv"
	@echo "###"
	/bin/cp -f tidl.csv _TIDL_origin/Code_Generator/Code_Generator.csv
	@echo "###"
	@echo "### start generating the code"
	@echo "###"
ifeq (,$(wildcard ./_TIDL_origin/Code_Generator/OUTPUT))
	cd _TIDL_origin/Code_Generator; make
else
	@echo "already exist ./_TIDL_origin/Code_Generator/OUTPUT"
endif
	rm -f _template
	rsync -arv --exclude="out" ${TEMPLATE}/${MODULENAME}-service/ _template
	cd ${TEMPLATE}/${MODULENAME}-service
	rm -rf _work
	rsync -arv --exclude="_*" --exclude=".project" --exclude=".mars" --exclude=".git" --exclude=".vscode" ${MYCODE} _work
	@echo "###"
	@echo "### start merging between _template and _work"
	@echo "###"
ifeq (,$(wildcard ./_merged))
	sh _TIDL_origin/CGA_RDL/3_recover.sh _template _work _merged
else
	@echo "already _merged ./_merged"
	@echo "if need new merge , remove ./_merged"
endif
	diff -urNEbBZ _work _merged > migration.patch || exit 0


merge_status:
	@echo "###"
	@echo "### start comparing"
	@echo "###"
	perl compare.pl --module=${MODULENAME} --checkdir=interface,service,sldd,uts --extension=h,hpp,cpp,c,csv,am --gitrevision=${DN_REVISION}
	@echo "###"
	@echo "work tidl version : ${REVISION}"
	@echo "template tidl version : `cat _merged/.tidl/revision`"


gen:
	@echo "###"
	@echo "### download and initialization"
	@echo "###"
ifeq (,$(wildcard ./_TIDL_origin))
	git clone http://mod.lge.com/hub/cheoljoo.lee/TIDL.git -b ${DN_REVISION} _TIDL_origin
	cd _TIDL_origin; make initdev
else
	cd _TIDL_origin/Code_Generator; git pull 
endif


update:
	make clean
	rm -rf "!(Makefile|modulename|revision|tidl.csv)"
	make gen
	/bin/cp -f tidl.csv _TIDL_origin/Code_Generator/Code_Generator.csv
	cd _TIDL_origin/Code_Generator; make
	rsync -arv --exclude="modulename" --exclude="revision" --exclude="tidl.csv" --exclude="Makefile" ${TEMPLATE}/${MODULENAME}-service/.tidl/* .
	make clean


migrate:
	cd ../; git apply .tidl/migration.patch
	make clean


clean:
	/bin/rm -rf _* *.log *.patch
