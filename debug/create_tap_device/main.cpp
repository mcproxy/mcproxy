#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
using namespace std;


int tun_alloc(char *dev, int flags, int userid) {
     struct ifreq ifr;
     int fd, err;
     char clonedev[] = "/dev/net/tun";
     /* Arguments taken by the function:
      * char *dev: the name of an interface (or '\0'). MUST have enough
      *space to hold the interface name if '\0' is passed
      * int flags: interface flags (eg, IFF_TUN etc.)
      */

     /* open the clone device */
     if( (fd = open(clonedev, O_RDWR)) < 0 ) {
          cout << "Can't open " << clonedev << "! Error: " << strerror(errno) << " errno: " << errno << endl;
          return -1;
     }
     /* preparation of the struct ifr, of type "struct ifreq" */
     memset(&ifr, 0, sizeof(ifr));
     ifr.ifr_flags = flags;
     /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

     if (*dev) {
          /* if a device name was specified, put it in the structure; otherwise,
           * the kernel will try to allocate the "next" device of the
           * specified type
           */
          strncpy(ifr.ifr_name, dev, IFNAMSIZ);
     }

     /* try to create the device */
     if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
          cout << "Can't set the tunnel! Error: " << strerror(errno) << " errno: " << errno << endl;
          return -1;
     }


     if(ioctl(fd, TUNSETPERSIST, 1) < 0){
          cout << "Can't set the tunnel persistent! Error: " << strerror(errno) << " errno: " << errno << endl;
     }

     if(ioctl(fd, TUNSETOWNER, userid) < 0){
          cout << "Can't set the owner! Error: " << strerror(errno) << " errno: " << errno << endl;
     }

     if(ioctl(fd, TUNSETGROUP, userid) < 0){
          cout << "Can't set the group! Error: " << strerror(errno) << " errno: " << errno << endl;
     }



     /* if the operation was successful, write back the name of the
      * interface to the variable "dev", so the caller can know
      * it. Note that the caller MUST reserve space in *dev (see calling
      * code below)
      */
     strcpy(dev, ifr.ifr_name);
     /* this is the special file descriptor that the caller will use to talk
      * with the virtual interface */
     return fd;
}

int tun_dealloc(char *dev, int flags) {
     struct ifreq ifr;
     int fd, err;
     char clonedev[] = "/dev/net/tun";
     /* Arguments taken by the function:
      * char *dev: the name of an interface (or '\0'). MUST have enough
      *space to hold the interface name if '\0' is passed
      * int flags: interface flags (eg, IFF_TUN etc.)
      */

     /* open the clone device */
     if( (fd = open(clonedev, O_RDWR)) < 0 ) {
          cout << "Can't open " << clonedev << "! Error: " << strerror(errno) << " errno: " << errno << endl;
          return -1;
     }
     /* preparation of the struct ifr, of type "struct ifreq" */
     memset(&ifr, 0, sizeof(ifr));
     ifr.ifr_flags = flags;
     /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

     if (*dev) {
          /* if a device name was specified, put it in the structure; otherwise,
           * the kernel will try to allocate the "next" device of the
           * specified type
           */
          strncpy(ifr.ifr_name, dev, IFNAMSIZ);
     }

     /* try to create the device */
     if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
          cout << "Can't set the tunnel! Error: " << strerror(errno) << " errno: " << errno << endl;
          return -1;
     }

     if(ioctl(fd, TUNSETPERSIST, 0) < 0){
          cout << "Can't reset the tunnel persistent! Error: " << strerror(errno) << " errno: " << errno << endl;
     }


     /* this is the special file descriptor that the caller will use to talk
      * with the virtual interface */
     return fd;
}

void help_output(){
     cout << "Usage: mcproxy [-h]" << endl;
     cout << "       mcproxy [-u <user/group id>][-d <tap name>]" << endl;
     cout << "       mcproxy [-c | -c <tap name>]" << endl;
     cout << endl;
     cout << "\t-h" << endl;
     cout << "\t\tDisplay this help screen." << endl;

     cout << "\t-d" << endl;
     cout << "\t\tDelete a tap device" << endl;

     cout << "\t-c" << endl;
     cout << "\t\tCreate a tap device." << endl;

}

int main(int argc, char *argv[])
{
     int tapfd = 0;
     int userid =0;
     char dev[IFNAMSIZ];
     memset(dev,0,sizeof(dev));


     if(argc == 1){

     }else{
          for (int c; (c = getopt(argc, argv, "hudc")) != -1;) {
               switch (c) {
               case 'h':
                    help_output();
                    return 0;
               case 'u':
                    if(argv[optind][0] != '-'){ //with an interface name
                         userid = atoi(&argv[optind][0]);
                         cout << "set userid: " << &argv[optind][0] << endl;
                    }else{
                         cout << "Wrong argument! See help (-h) for more information" << endl;
                         return 0;
                    }
                    break;
               case 'd':
                    if(optind < argc){ //with an interface name
                         strncpy(dev, &argv[optind][0], sizeof(dev));
                         cout << "set device name: " << dev << endl;
                    }

                    if(tapfd = tun_dealloc(dev,IFF_TAP) >0){
                         cout << "device deleted from the kernel: " << dev << endl;
                    }

                    return 0;
               case 'c':
                    if(optind < argc){ //with an interface name
                         strncpy(dev, &argv[optind][0], sizeof(dev));
                         cout << "set device name: " << dev << endl;
                    }

                    if(tapfd = tun_alloc(dev,IFF_TAP,userid) >0){
                         cout << "device created by the kernel: " << dev << endl;
                    }

                    return 0;
               default:
                    cout << "Unknown argument! See help (-h) for more information.";
                    return 0;
               }
          }
     }

     cout << "Nothing done! See help (-h) for more information.t" << endl;
     return 0;
}
