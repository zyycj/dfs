// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include <fcntl.h>
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "extent_client.h"

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst) {
    ec = new extent_client(extent_dst);
    srand(time(0));
    // ec->put(1, "");
}

yfs_client::inum yfs_client::n2i(std::string n) {
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string yfs_client::filename(inum inum) {
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool yfs_client::isfile(inum inum) {
    if (inum & 0x80000000) return true;
    return false;
}

bool yfs_client::isdir(inum inum) { return !isfile(inum); }

<<<<<<< HEAD
int yfs_client::getfile(inum inum, fileinfo &fin) {
    int r = OK;
=======
int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock
>>>>>>> 8a7574edd36e5062d002cd3aa54e723fb24e69ad

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:

    return r;
}


int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

int yfs_client::truncate(inum file, size_t size) {
    std::string file_buf;
    ec->get(file, file_buf);
    if(size < file_buf.size()) {
        file_buf = file_buf.substr(0, size);
    }
    else {
        file_buf += std::string(size - file_buf.size(), '\0');
    }
    ec->put(file, file_buf);
    return OK;
}

int yfs_client::read(inum file, std::string& buf, size_t size, size_t off) {
    std::string file_buf;
    extent_protocol::attr attr;
    ec->getattr(file, attr);
    if(off >= attr.size) {
        return OK; 
    }
    ec->get(file, file_buf);
    buf = file_buf.substr(off, size);
    return OK;
}
int yfs_client::write(inum file, std::string buf, size_t size, size_t off) {
    std::string file_buf;
    std::string new_write_buf;
    ec->get(file, file_buf);
    if(off < file_buf.size()) {
        std::string buf1 = file_buf.substr(0, off);
        std::string buf2 = buf.substr(0, size);
        new_write_buf = buf1 + buf2;
        if(off + size < file_buf.size()) {
            new_write_buf += file_buf.substr(off + size);
        } 
    } else {
        std::string hole = std::string(off - file_buf.size(), '\0'); 
        new_write_buf = file_buf + hole + buf.substr(0, size);
    }
    ec->put(file, new_write_buf);
    return OK;
}

yfs_client::inum yfs_client::random_ino_() {
    inum ino = (inum)rand() | 0x80000000;
    return ino;
}

int yfs_client::create(inum parent, std::string name, inum &ret_ino) {
    inum ino;
    std::string parent_buf;
    if (lookup(parent, name, ino) == OK) {
        return EXIST;
    }
    ino = random_ino_();
    ec->put(ino, "");

    ec->get(parent, parent_buf);
    parent_buf.append(name + ":" + std::to_string(ino) + ",");
    ec->put(parent, parent_buf);
    ret_ino = ino;
    return OK;
}

int yfs_client::lookup(inum parent, std::string name, inum &ret_ino) {
    std::string buf;
    std::string::size_type pos = -1, last_pos = -1;
    auto r = ec->get(parent, buf);
    if (r != OK) return IOERR;
    while ((pos = buf.find(",", pos + 1)) != std::string::npos) {
        std::string file_entry = buf.substr(last_pos + 1, pos - last_pos - 1);
        auto divide_pos = file_entry.find(":");
        auto file_name = file_entry.substr(0, divide_pos);
        auto file_ino = n2i(file_entry.substr(divide_pos + 1));
        last_pos = pos;
        if (file_name == name) {
            ret_ino = file_ino;
            return OK;
        }
    }
    return NOENT;
}

int yfs_client::readdir(inum dir, std::map<std::string, inum> &files) {
    std::string buf;
    std::string::size_type pos = -1, last_pos = -1;
    // bool is_parent_name = true;
    auto r = ec->get(dir, buf);
    if (r != OK) return IOERR;
    while ((pos = buf.find_first_of(",", pos + 1)) != std::string::npos) {
        std::string file_entry = buf.substr(last_pos + 1, pos - last_pos - 1);
        auto divide_pos = file_entry.find(":");
        auto file_name = file_entry.substr(0, divide_pos);
        auto file_ino = n2i(file_entry.substr(divide_pos + 1));
        last_pos = pos;
        files[file_name] = file_ino;
    }
    return OK;
}

