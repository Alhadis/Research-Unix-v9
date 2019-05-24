#!/bin/sh
cat <<'EOF'
#!/bin/sh
#
# program to set up symbolic or hard links the way you need them
#
linktype=
while :
do
	echo -n "Do you want symbolic links (if no, you get hard links)? "
	read ans
	case "$ans" in
	y*)	linktype=symbolic
		break
		;;
	n*)	break
		;;
	*)	echo "type y or n"
		;;
	esac
done

while read file linkto type
do
	case "$linktype" in
	symbolic)
		echo ln -s $linkto $file
		ln -s $linkto $file
		;;
	*)
		case "$type" in
		file)
			linkto=`echo "$file $linkto" | sed -e 's/\/[^\/]* /\//'`
			echo ln $linkto $file
			ln $linkto $file
			;;
		directory)
			echo "mkdir $file; cd $file; ln ../$linkto/* ."
			(mkdir $file; cd $file; ln ../$linkto/* .)
			;;
		esac
		;;
	esac
done <<funky-EOF
EOF

trap "rm -f $TMP; exit 0" 0 1 2
TMP=/tmp/mklnk.$$
(
	cd $1
	find . -type l -print \
	| while read file
	do
		if [ -d $file ]
		then
			type=directory
		else
			type=file
		fi
		linkto=`ls -ld $file | sed -e 's/.* -> //'`
		case "$type" in
		directory)
			echo $file $linkto $type
			;;
		file)
			echo $file $linkto $type >> $TMP
			;;
		esac
	done
	cat $TMP
)
echo funky-EOF
