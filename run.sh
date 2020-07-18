#!/bin/sh                                                                       
D=`pwd`                                                                         
for i in `ls $D/images_eink`                                                    
do                                                                              
echo  $D/images_eink/$i                                                         
./test_bmp_eink -x -f $D/images_eink/$i                                         
sleep 2                                                                         
done
