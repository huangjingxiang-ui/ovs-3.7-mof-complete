#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE     1024
#define READNUM     200
#define OVS_PATH "/home/mof/ovs-2.16.0"


char readfile[READNUM][200] = { 
                                "./PyPrj/cfile/ori_flow_pad.c", 
                                "./save_file/flow_h_save2", 
                                "./PyPrj/cfile/header.c", 
                                "./PyPrj/cfile/smallstruct.c",
                                "./PyPrj/cfile/unionprotols.c",
                                "./PyPrj/cfile/enumpmap.c",
                                "./PyPrj/cfile/flow.h",

								"./PyPrj/cfile/flowmap_set_mof.c",
								"./save_file/flow_c_save2",
                                "./PyPrj/cfile/parser.c",
                                "./PyPrj/cfile/miniflow_extract.c",

                                "./PyPrj/cfile/odp-execute/case_SEG_ROUTING.c", 
                                "./save_file/odp-execute_c_save2",

                                "./PyPrj/cfile/packets/packets_c_1.c", 
                                "./save_file/packets_c_save2",
                                "./PyPrj/cfile/packets/packets_c_2.c", 
                                "./save_file/packets_c_save3",

                                "./PyPrj/cfile/ofp-actions.c", 
                                "./save_file/ofp-actions_c_save2",

                                "./PyPrj/cfile/ofproto-dpif-xlate/xlate-offset.c",
                                "./save_file/ofproto-dpif-xlate_c_save2",
                                "./PyPrj/cfile/ofproto-dpif-xlate/xlate-srv6.c",
                                "./save_file/ofproto-dpif-xlate_c_save3",

                                "./PyPrj/cfile/meta-flow.c",
                                "./PyPrj/cfile/hash.c",

                                "./PyPrj/cfile/match.c", 
                                "./save_file/match_c_save2",

                                "./PyPrj/cfile/openflow-1.0.h",
                                "./save_file/openflow-1.0_h_save2",

                                "./PyPrj/javafile/TrafficSelector_import.java",
                                "./save_file/TrafficSelector_java_save2",
                                "./PyPrj/javafile/TrafficSelector.java",
                                "./save_file/TrafficSelector_java_save3",

                                "./PyPrj/javafile/criteria/Criterion.java",
                                "./save_file/Criterion_java_save2",

                                "./PyPrj/javafile/criteria/criteria_import.java",
                                "./save_file/Criteria_java_save2",
                                "./PyPrj/javafile/criteria/Criteria.java",
                                "./save_file/Criteria_java_save3",

                                "./PyPrj/javafile/DefaultTrafficSelector_import.java",
                                "./save_file/DefaultTrafficSelector_java_save2",
                                "./PyPrj/javafile/DefaultTrafficSelector1.java",
                                "./save_file/DefaultTrafficSelector_java_save3",
                                "./PyPrj/javafile/DefaultTrafficSelector2_01.java",
                                "./save_file/DefaultTrafficSelector_java_save4",
                                "./PyPrj/javafile/DefaultTrafficSelector2_02.java",
                                "./PyPrj/javafile/DefaultTrafficSelector2_1.java",
                                "./PyPrj/javafile/DefaultTrafficSelector2_2.java",
                                "./save_file/DefaultTrafficSelector_java_save5",
                               }; 

char hpath[BUFSIZE] = "/include/openvswitch/flow.h";
char cpath[BUFSIZE] = "/lib/flow.c";

//strcat(OVS_PATH,hpath);

char appendfile[READNUM][200] = {  
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",
                                "./tmp_file/flow.h",

                                "./tmp_file/flow.c",
                                "./tmp_file/flow.c",
								"./tmp_file/flow.c",
                                "./tmp_file/flow.c",

                                "./tmp_file/odp-execute.c",
                                "./tmp_file/odp-execute.c",

                                "./tmp_file/packets.c",
                                "./tmp_file/packets.c",
                                "./tmp_file/packets.c",
                                "./tmp_file/packets.c",

                                "./tmp_file/ofp-actions.c",
                                "./tmp_file/ofp-actions.c",

                                "./tmp_file/ofproto-dpif-xlate.c",
                                "./tmp_file/ofproto-dpif-xlate.c",
                                "./tmp_file/ofproto-dpif-xlate.c",
                                "./tmp_file/ofproto-dpif-xlate.c",

                                "./tmp_file/meta-flow.c",
                                "./tmp_file/flow.c",

                                "./tmp_file/match.c",
                                "./tmp_file/match.c",

                                "./tmp_file/openflow-1.0.h",
                                "./tmp_file/openflow-1.0.h",

                                "./tmp_file_java/TrafficSelector.java",
                                "./tmp_file_java/TrafficSelector.java",
                                "./tmp_file_java/TrafficSelector.java",
                                "./tmp_file_java/TrafficSelector.java",

                                "./tmp_file_java/criteria/Criterion.java",
                                "./tmp_file_java/criteria/Criterion.java",

                                "./tmp_file_java/criteria/Criteria.java",
                                "./tmp_file_java/criteria/Criteria.java",
                                "./tmp_file_java/criteria/Criteria.java",
                                "./tmp_file_java/criteria/Criteria.java",


                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                "./tmp_file_java/DefaultTrafficSelector.java",
                                  };
char end[] = "#ifdef __cplusplus\n\
}\n\
#endif\n\
#endif\n";

int main()
{
    FILE *wfp;
    int rfd,len;
    char buf[BUFSIZE];
    int beforelen = 0;
    //test pos
    //fputs("i am here", wfp);

    for(int i = 0; i < READNUM; i++)
    {   
        //打开文件
        if(appendfile[i][0] == '\0')
            continue;

        wfp = fopen(appendfile[i], "r+");
        if(wfp == NULL)
        {
            perror("fopen()");
            printf("%s",appendfile[i]);
            exit(0);
        }
        if(i != 0)
            beforelen = 0;
        fseek(wfp,beforelen,SEEK_END);

        //读文�?
        if(readfile[i][0] == '\0')
            continue;
        rfd = open(readfile[i],O_RDONLY);
        if(rfd < 0)
        {
            printf("%s\n",readfile[i]);
            perror("open()");
            exit(0);
        }
        while(1)
        {
            while((len = read(rfd,buf,BUFSIZE)) < 0)
            {
                if(errno == EINTR)
                    continue;
                perror("read");
                break;
            }

            if(len == 0)
                break;
            
            //写文�?
            fwrite(buf,len,1,wfp);
        }

        if(i == 6)
        {
            fwrite(end,34,1,wfp);
        }

        close(rfd);
        fclose(wfp);
    }

    
    exit(0);
}