original_pwd=`pwd`
dir=$(dirname $2)
cd $dir
path2=`pwd`/$(basename $2)
cd $original_pwd
dir=$(dirname $0)
cd $dir/../../data
mkdir -p phoneless
cd phoneless
if [ -d $1 ]
then echo "This id $1 has existed."
else mkdir $1
fi
cd $1
../../../src/phoneless/a.out $path2
../../../src/phoneless/b.out
../../../src/phoneless/c.out
# echo $1, $path2, $path3
# echo `pwd`

