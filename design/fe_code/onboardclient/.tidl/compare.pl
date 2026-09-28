#!/bin/perl
# vim backup : https://medium.com/@Aenon/vim-swap-backup-undo-git-2bf353caa02f
# vim backup setting : https://gist.github.com/nepsilon/003dd7cfefc20ce1e894db9c94749755
#
# how-to-count-differences-between-two-files-on-linux : https://stackoverflow.com/questions/1566461/how-to-count-differences-between-two-files-on-linux
#
use File::Compare;
use Getopt::Long;

$number_args = $#ARGV + 1;


sub help
{
	printf("Help :\n");
	printf("\t\n");
	printf("\tGeneral Calculation : template-work + work-merge + template-work\n");
	printf("\t\n");
	printf("\t--search=[your_wanted_substring]\n");
	printf("\t\t  do with files including this search substring\n");
	printf("\t--12\n");
	printf("\t\t  sort the diff count between template and work\n");
	printf("\t\t  meaning : violation + developer's works\n");
    printf("\t\t          : developers changed a lot of CGA_VARIANT_START ~ END including violation\n");
	printf("\t--23\n");
	printf("\t\t  sort the diff count between work and merged\n");
	printf("\t\t  meaning : violation\n");
	printf("\t\t          : it has big potential  violation. already all developer's works are applied in merged. \n");
	printf("\t--13\n");
	printf("\t\t  sort the diff count between template and merged\n");
	printf("\t\t  it is the same as --31\n");
	printf("\t--31\n");
	printf("\t\t  sort the diff count between merged and template\n");
	printf("\t\t  meaning : developer's works\n");
	printf("\t\t          : developers changed a lot of CGA_VARIANT_START ~ END\n");
	printf("\t--help\n");

    print_options();
}

sub print_compare
{
        my $mycnt = shift;
        printf("%4d : %-70s template - ",$mycnt, $filelist{$mycnt});
        printf("[%5d] - work - ", $filediff{$mycnt}{12});
        printf("[%5d] - merged - ", $filediff{$mycnt}{23});
        printf("[%5d] - template", $filediff{$mycnt}{13});
        my $shortname = $filelist{$mycnt};
        if($shortname =~ /([^\/\s]+)\s*$/){
            $shortname = $1;
        }
        printf(" : %-20s", $shortname);
        print "\n"
}

sub print_options
{
    print("option_module=tidl -> $option_module\n");
    print("option_checkdir=service,interface,sldd -> $option_checkdir : @option_checkdir_list\n");
    print("option_extension=$option_extension : @option_extension_list\n");
    print("option_gitrevision=$option_gitrevision\n");
}

our $option_module="tidl";
our $option_checkdir="service,interface,sldd";
our $option_extension="hpp,h,am,";
our $option_gitrevision="";
our $option_checkdir_list;
our @option_extension_list;
our %option_extension_dic;
our $sort12=0;
our $sort23=0;
our $sort13=0;
our $search = "";
GetOptions (
        "search=s"   => \$search,      # string
        "12" => sub { $sort12 = 1 },   # flag
        "23" => sub { $sort23 = 1 },   # flag
        "31" => sub { $sort13 = 1 },   # flag
        "13" => sub { $sort13 = 1 },   # flag
        "module=s" => \$option_module,
        "checkdir=s" => \$option_checkdir,
        "extension=s" => \$option_extension,
        "gitrevision=s" => \$option_gitrevision,
		"verbose|help"  => sub { help(); exit(); })   # flag
or  die(help() . "\nError in command line arguments\n");


@option_checkdir_list = split(',' , $option_checkdir);
@option_extension_list = split(',' , $option_extension);
foreach my $oe (@option_extension_list){
    $option_extension_dic{$oe} = 1;
}
print_options();


print "==start==\n";
my $dirStart = "./_template";
my @dirs;  #= ("$dirStart/common" , "$dirStart/src" , "$dirStart/sldd" , "$dirStart/include");
my $cnt = 0;
foreach my $oc (@option_checkdir_list){
    $dirs[$cnt] = "$dirStart\/$oc";
    print("$cnt $dirs[$cnt] $oc\n");
    $cnt++;
}
print("@dirs\n\n");
print(%option_extension_dic);
print("\n\n");
my %seen;
my %files;
our %filelist;
our %filediff;
while (my $pwd = shift @dirs) {
        opendir(DIR,"$pwd") or die "Cannot open $pwd\n";
        my @files = readdir(DIR);
        closedir(DIR);
        foreach my $file (@files) {
                next if $file =~ /^\.\.?$/;
                my $path = "$pwd/$file";
                if (-d $path) {
                    next if $seen{$path};
                    $seen{$path} = 1;
                    push @dirs, $path;
                } else {
					#next if ($path !~ /\.txt$/i);
					my $mys = $path;
                    if( ($search ne "") && (not ($mys =~ /$search/)) ){
                        next;
                    }
                    $mys =~ /\.([^\.]+)$/;
                    $myext = $1;
                    if($option_extension_dic{$myext} == 1){
						$mys =~ s/^$dirStart\///;
						my $myshort = $mys;
						if($mys =~ /\/([^\/]*)$/){
							$myshort = $1;
						}
						$files{$mys} = $myshort;
                        print "1 $mys : $myshort\n";
					}
					my $mtime = (stat($path))[9];
					#print "$path $mtime\n";
				}
        }
}

$cnt = 1;
foreach my $mys (sort keys %files){
    print ".";
    #print "file: $mys : $files{$mys} -> \n";
    my $myresult;
    my $flag = 0;
    $myresult = compare("./_template/$mys","./_work/$mys");
    if ($myresult == 0) {
        #print "12 $mys : $files{$mys} => They're equal between template and merged\n";
    } elsif($myresult == 1){
        #print "12[$cnt] DIFF : $mys : $files{$mys} => why do you change it? we can not allow it.\n";
        $filelist{$cnt} = $mys;
        $filediff{$cnt}{12} = `diff ./_template/$mys ./_work/$mys --suppress-common-lines --speed-large-files -y -E -b -B -Z | wc -l`;
        chop($filediff{$cnt}{12});
        $flag = 1;
    }
    $myresult = compare("./_work/$mys","./_merged/$mys");
    if ($myresult == 0) {
        #print "23 $mys : $files{$mys} => They're equal between template and merged\n";
    } elsif($myresult == 1){
        #print "23[$cnt] DIFF : $mys : $files{$mys} => why do you change it? we can not allow it.\n";
        $filelist{$cnt} = $mys;
        $filediff{$cnt}{23} = `diff ./_work/$mys ./_merged/$mys --suppress-common-lines --speed-large-files -y -E -b -B -Z | wc -l`;
        chop($filediff{$cnt}{23});
        $flag = 1;
    }
    $myresult = compare("./_template/$mys","./_merged/$mys");
    if ($myresult == 0) {
        #print "23 $mys : $files{$mys} => They're equal between template and merged\n";
    } elsif($myresult == 1){
        #print "23[$cnt] DIFF : $mys : $files{$mys} => why do you change it? we can not allow it.\n";
        $filelist{$cnt} = $mys;
        $filediff{$cnt}{13} = `diff ./_template/$mys ./_merged/$mys --suppress-common-lines --speed-large-files -y -E -b -B -Z | wc -l`;
        chop($filediff{$cnt}{13});
        $flag = 1;
    }
    if($flag == 1){ $cnt++; }
}

my $prefix="";
my $choose=0;
my $mysubstr="";
while(1){
    if($sort12){
        print "\n\n (12) DIFF LIST :: sort template - work\n";
        foreach my $mycnt (sort{$filediff{$b}{12}<=>$filediff{$a}{12}} keys %filelist){
            if( ($mysubstr ne "") && (not($filelist{$mycnt} =~ /$mysubstr/)) ) { next; }
            print_compare($mycnt);
        }
    } elsif($sort23){
        print "\n\n (23) DIFF LIST :: sort work - merged\n";
        foreach my $mycnt (sort{$filediff{$b}{23}<=>$filediff{$a}{23}} keys %filelist){
            if( ($mysubstr ne "") && (not($filelist{$mycnt} =~ /$mysubstr/)) ) { next; }
            print_compare($mycnt);
        }
    } elsif($sort13){
        print "\n\n (13 or 31) DIFF LIST :: sort template - merged\n";
        foreach my $mycnt (sort{$filediff{$b}{13}<=>$filediff{$a}{13}} keys %filelist){
            if( ($mysubstr ne "") && (not($filelist{$mycnt} =~ /$mysubstr/)) ) { next; }
            print_compare($mycnt);
        }
    } else {
        print "\n\n (default) DIFF LIST :: sort index\n";
        foreach my $mycnt (sort{$a<=>$b} keys %filelist){
            if( ($mysubstr ne "") && (not($filelist{$mycnt} =~ /$mysubstr/)) ) { next; }
            print_compare($mycnt);
        }
    }
    print "prefix[$prefix] choose[$choose] substr[$mysubstr]\n";
    print "\n";
    print "   q  is quit\n";
    my $myc = $choose+1;
    print "   <CR> $prefix$myc\n";
    print "   [index#]          ex) 3   : 3 diff\n";
    print "   m[index#]         ex) m3  : 2 diff between template and merged\n";
    print "   s=[your_substring ex) s=sldd-  : show files including substring\n";
    print "   rt : exist template and not exist work\n";
    print "   rw : exist work and not exist template\n";
    print "press the number of your wanted file index : ";
    $in = <>;
    chop($in);
    print "in[$in]\n";
    if($in eq "-1"){ exit; }
    if($in eq "quit"){ exit; }
    if($in eq "q"){ exit; }
    if($in eq "rt"){
        my $dirStart = "./_template";
        my @dirs;  #= ("$dirStart/common" , "$dirStart/src" , "$dirStart/sldd" , "$dirStart/include");
        my $cnt = 0;
        foreach my $oc (@option_checkdir_list){
            $dirs[$cnt] = "$dirStart\/$oc";
            #print("$cnt $dirs[$cnt] $oc\n");
            $cnt++;
        }
        #print("@dirs\n\n");
        #print(%option_extension_dic);
        ##print("\n\n");
        my %seen;
        my %tempfiles;
        while (my $pwd = shift @dirs) {
            opendir(DIR,"$pwd") or die "Cannot open $pwd\n";
            my @files = readdir(DIR);
            closedir(DIR);
            foreach my $file (@files) {
                next if $file =~ /^\.\.?$/;
                my $path = "$pwd/$file";
                if (-d $path) {
                    next if $seen{$path};
                    $seen{$path} = 1;
                    push @dirs, $path;
                } else {
                    #next if ($path !~ /\.txt$/i);
                    my $mys = $path;
                    $mys =~ /\.([^\.]+)$/;
                    $myext = $1;
                    if($option_extension_dic{$myext} == 1){
                        $mys =~ s/^$dirStart\///;
                        my $myshort = $mys;
                        if($mys =~ /\/([^\/]*)$/){
                            $myshort = $1;
                        }
                        $tempfiles{$mys} = $myshort;
                    }
                }
            }
        }

        foreach my $mys (sort keys %tempfiles){
            if(not(-e "./_work/$mys")){
                print "./_work/$mys is not exist\n";
            }
        }
        print "press the return key";
        <>;
        next;
    }
    if($in eq "rw"){
        my $dirStart = "./work";
        my @dirs;  #= ("$dirStart/common" , "$dirStart/src" , "$dirStart/sldd" , "$dirStart/include");
        my $cnt = 0;
        foreach my $oc (@option_checkdir_list){
            $dirs[$cnt] = "$dirStart\/$oc";
            #print("$cnt $dirs[$cnt] $oc\n");
            $cnt++;
        }
        #print("@dirs\n\n");
        #print(%option_extension_dic);
        #print("\n\n");
        my %seen;
        my %tempfiles;
        while (my $pwd = shift @dirs) {
            opendir(DIR,"$pwd") or die "Cannot open $pwd\n";
            my @files = readdir(DIR);
            closedir(DIR);
            foreach my $file (@files) {
                next if $file =~ /^\.\.?$/;
                my $path = "$pwd/$file";
                if (-d $path) {
                    next if $seen{$path};
                    $seen{$path} = 1;
                    push @dirs, $path;
                } else {
                    #next if ($path !~ /\.txt$/i);
                    my $mys = $path;
                    $mys =~ /\.([^\.]+)$/;
                    $myext = $1;
                    if($option_extension_dic{$myext} == 1){
                        $mys =~ s/^$dirStart\///;
                        my $myshort = $mys;
                        if($mys =~ /\/([^\/]*)$/){
                            $myshort = $1;
                        }
                        $tempfiles{$mys} = $myshort;
                    }
                }
            }
        }

        foreach my $mys (sort keys %tempfiles){
            if(not(-e "./_template/$mys")){
                print "./_template/$mys is not exist\n";
            }
        }
        print "press the return key";
        <>;
        next;
    }
    if($in =~ /^s*$/){
        $choose = $choose + 1;
        if($prefix eq "m"){
            system("vimdiff ./_template/$filelist{$choose} ./_merged/$filelist{$choose}");
        } else {
            system("vimdiff ./_template/$filelist{$choose} ./_work/$filelist{$choose} ./_merged/$filelist{$choose}");
        }
    } elsif($in =~ /^\s*s=(\S*)\s*$/){
        $mysubstr = $1;
    } else {
        if($in =~ /^(m?)(\d+)$/){
            $prefix = $1;
            $choose = $2;
        }
        if($prefix eq "m"){
            system("vimdiff ./_template/$filelist{$choose} ./_merged/$filelist{$choose}");
        } else {
            system("vimdiff ./_template/$filelist{$choose} ./_work/$filelist{$choose} ./_merged/$filelist{$choose}");
        }
    }
}
