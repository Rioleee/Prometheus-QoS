/* Modified by: xChaos, 20130115 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define STRLEN 512

/* globals declared in prometheus.c */
extern struct IP *ips, *ip, *sharedip;
extern char *mark;
extern char *proxy_ip;
extern int free_min;
extern int free_max;
extern int include_upload;

/* ===================== traffic analyser - uses iptables  ================ */ 

void get_traffic_statistics(const char *whichiptables)
{
 char *str,*cmd;
 int downloadflag=0;

 textfile(Pipe,str) *line,*lines=NULL;
 string(str,STRLEN);
 string(cmd,STRLEN);

 sprintf(cmd,"%s -L -v -x -n -t mangle", whichiptables);
 shell(cmd);
 input(str,STRLEN)
 {
  create(line,Pipe);
  line->str=str;
  string(str,STRLEN);
  append(line,lines);
 }

 for_each(line,lines)
 {
  int col, accept = 0, proxyflag = 0, valid = 1, setchainname = 0, commonflag = 0; 
  unsigned long long traffic = 0;
  unsigned long pkts = 0;
  char *ipaddr = NULL,*ptr;
  
  valid_columns(ptr, line->str, ' ', col) 
  if(valid) switch(col)
  { 
   case 1: if(eq(ptr,"Chain"))
           {
            setchainname = 1;
           }
           else if(eq(ptr,"pkts")) 
           {
            valid = 0;
           }
           else
           {
            sscanf(ptr,"%lu",&pkts); 
           }
           break;
   case 2: if(setchainname)
           {
            if(!strncmp(ptr,"post_",5) || eq(ptr,"POSTROUTING"))
            {
             downloadflag = 1;            
            }
            else 
            {
             if(!strncmp(ptr,"forw_",5) || eq(ptr,"FORWARD"))
             {
              downloadflag = 0;
             }
            }            
            if(eq(ptr,"post_common") || eq(ptr,"forw_common"))
            {
             commonflag = 1;
            }
           }
           else
           {
            sscanf(ptr,"%Lu",&traffic); 
            traffic += (1<<19);
            traffic >>= 20;
           }
           break;
   case 3: if((strncmp(ptr,"post_",5) && strncmp(ptr,"forw_",5)) || commonflag)
           {
            accept = eq(ptr,mark);
           }
            /*if(filter_type==1) accept=eq(ptr,"MARK"); else accept=eq(ptr,"CLASSIFY");*/
           break;
   case 8: if(downloadflag)
           { 
            if(strstr(proxy_ip,ptr))
            {
             proxyflag = 1;
            }
           }
           else
           {
            ipaddr = ptr;
           }
           break;
   case 9: if(downloadflag)ipaddr = ptr;break;
  }
  
  if(accept && traffic>0 && ipaddr)
  {
   if(proxyflag)
   {
    printf("(proxy) ");
   }
   else if(!downloadflag)
   {
    printf("(upload) ");
   }
   printf("IP %s: %Lu MB (%ld pkts)\n", ipaddr, traffic, pkts);

   if_exists(ip,ips,eq(ip->addr,ipaddr)); 
   else 
   {
    TheIP(ipaddr);
    if(eq(ip->addr,"0.0.0.0/0"))
    {
     ip->name = "(unregistered)";
     ip->min = free_min;
     ip->max = ip->desired=free_max;
    }
    else
    {
     ip->name = ipaddr;
    }
   }
   
   if(downloadflag)
   {
    if(proxyflag)
    {
     ip->proxy = traffic;
    }
    else
    {
     ip->traffic += traffic;
    }
    ip->direct = ip->traffic-ip->upload-ip->proxy;
    ip->pktsdown = pkts;
   }
   else
   {
    ip->upload = traffic;
    ip->pktsup = pkts;
    if(include_upload)
    {
     ip->traffic += traffic;
    }
    else 
    {
     if(traffic > ip->traffic)
     {
      ip->traffic = traffic;     
     }
    }
   }
  }  
 }
 free(cmd);
}