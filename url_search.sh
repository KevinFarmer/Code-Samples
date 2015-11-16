#!/bin/bash
#Script name: url_search.sh
#
#Description:Searches for the given arguments at
#at the given URLs and counts them
#
#Input: a text file containing a list of URLs
#
#Example Usage
# ./url_search.sh urlFile.txt searchWord1 searchWord2
#
#
#Output: Each argument and the count of it at each URL
#
#Special considerations: This script takes a text file which
#must contain only URLs
#
#Psuedocode: Download the given webpages, and for each word
#search through each webpage and find the count.
#Then remove the downloaded pages
#

if [ $# -eq 0 ]
    then
    printf "Must enter arguments\n"
    exit 1
fi

#check if it is a text file
urls=`cat $1 2>/dev/null`
if [ $? != 0 ] 
    then
    printf "Enter an existing text file\n"
    exit 1
fi

#check if file is empty
if [ ${#urls} == 0 ]
    then
    printf "File contains no URLs\n"
    exit 1
fi

#check for arguments
if [ ${#@} == 1 ]
    then
    printf "No given search words\n"
    exit 1
fi


dir=`mktemp -d`

#remove temp files
removeFiles () {
while [ $numURL != 0 ]; do
    rm -f "${dir}/${numURL}.html"
    let numURL-=1
done
}


#download each webpage
numURL=1
for url in $urls; do
    curl $url -s -L -o "${dir}/${numURL}.html"
    if [ $? -ne 0 ]
    then
	printf "${url} is not a valid URL. \n"
	removeFiles
	exit 1
    fi
    let numURL+=1
done
wait


#start at $2, the first search word
searches=`echo "${*:2}"`
for word in $searches; do
    echo $'\n'$word

    i=1 #track num associated with .html file for url
    for url in $urls; do
	printf "$url `grep -o $word <"${dir}/${i}.html" | wc -l` \n"
	let i+=1
    done
done


exit 0
