#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

int mounted = 0;

uint32_t encode_op(jbod_cmd_t cmd, int disk_num, int block_num) {
    uint32_t op = 0;
    op = (cmd << 26 | disk_num << 22 | block_num); // or command

    return op;
}
int min(int a, int b) {
    if (a > b) {
        return b;
    } else {
        return a;
    }
}
int mdadm_mount(void) {
    if (mounted) {
        return -1;
    }

    // mounts all of the lower digits onto the upper digits, mount doesnt care
    // about disk_num or block_num
    //
    uint32_t op = encode_op(JBOD_MOUNT, 0, 0);
    
    jbod_client_operation(op, NULL); // fails if return -1

    mounted = 1;
    return 1;
}

int mdadm_unmount(void) {
    if (mounted == 0) {
        return -1;
    }
    uint32_t op = encode_op(JBOD_UNMOUNT, 0, 0);
    jbod_client_operation(op, NULL);
    mounted = 0;
    return 1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    if (mounted == 0) {
        return -1;
    }
    if (len > 1024) {
        return -1;
    }
    if (buf == NULL && len > 0) {
        return -1;
    }
    if (addr + len > 1048576) {
        return -1;
    }
    uint32_t temp_len = len;
    int flag =0;
    int num_read = 0;
    uint8_t mybuf[256];
    int blocks_read=0;
    // uint32_t curr_addr = addr;
    while (num_read < len) {
            //reads less any amount of data less than 256 for first read
        if ( temp_len <= 256 && num_read == 0 ) {
            int disk_num = addr / JBOD_DISK_SIZE;

            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);
            // jbod_operation(encode_op(JBOD_WRITE_BLOCK,0,0),mybuf);
            int num_bytes_to_read_from_block =min(temp_len,(256-(addr%256)));
            memcpy(buf + num_read, mybuf ,num_bytes_to_read_from_block);
            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
            
            
        } 
        else if (num_read ==0 && temp_len >256){
            int disk_num = addr / JBOD_DISK_SIZE;

           // buffer_offset = addr % 256;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);
            int num_bytes_to_read_from_block = 256-(addr%256);

            memcpy(buf , mybuf ,num_bytes_to_read_from_block);
            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
        }
        //read whole block from beginning
        else if ((temp_len ) >= 256 && num_read > 0 && addr%256==0) {
            int disk_num = addr / JBOD_DISK_SIZE;

            //buffer_offset = addr % 256;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);

            int num_bytes_to_read_from_block = 256;

            memcpy(buf +num_read, mybuf ,num_bytes_to_read_from_block);

            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
        }
        // will read the remaining bytes of the command, from a beginning of a block
        else if(temp_len<256 &&num_read>0 && addr%256==0){
            int disk_num = addr / JBOD_DISK_SIZE;

            //buffer_offset = addr % 256;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);

            int num_bytes_to_read_from_block = temp_len;

            memcpy(buf +num_read, mybuf ,num_bytes_to_read_from_block);

            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
        }
        
            
        else if( temp_len>256 && addr%256!=0){
            int disk_num = addr / JBOD_DISK_SIZE;

            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;

            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);
            int num_bytes_to_read_from_block = 256-(addr%256);
            memcpy(buf + num_read, mybuf ,num_bytes_to_read_from_block);

            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
        }
        else if (temp_len<256){
            int disk_num = addr / JBOD_DISK_SIZE;

            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;

            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);
            int num_bytes_to_read_from_block =256-(addr%256);
           
            memcpy(buf + num_read, mybuf ,num_bytes_to_read_from_block);

            num_read += num_bytes_to_read_from_block;
            temp_len -= num_bytes_to_read_from_block;
            addr += num_bytes_to_read_from_block;
            flag=1;
            blocks_read=1;
        }
        if (flag ==0 && blocks_read>0){
            break;
        }
    }
    return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
    if (len > 1024) {
        return -1;
    }
    if (buf == NULL && len > 0) {
        return -1;
    }
    if (addr + len > 1048576) {
        return -1;
    }
    

    if (mounted == 0) {
        return -1;
    }

    uint32_t temp_len = len;
    uint8_t mybuf[256];
    int buffer_offset = 0;

    int block_offset = addr % 256;
    int block_wrote = 0;
    int curr_addr = addr;
    int num_written = 0;
    // while(num_written < temp_len){
    int flag = 0;
    while (num_written < len) {
        // first block to write, len is less  or equal than a block, starts from block beginning
        if (block_wrote == 0 && block_offset == 0 && temp_len <= 256) {
            int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);

            memcpy(mybuf + block_offset, buf + buffer_offset, temp_len);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);
            int num_byte_to_write = temp_len ;
            block_wrote += 1;
            curr_addr += num_byte_to_write;
            buffer_offset = 256 - buffer_offset;
            num_written += num_byte_to_write;
            temp_len -= num_written;
            addr+=num_byte_to_write;
            flag = 1;
        }
        // no blocks written, starting somewhere in block, writes less than block
        else if (block_wrote==0 && block_offset>0 && temp_len<=256){
             int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);

            memcpy(mybuf + block_offset, buf + buffer_offset, min(temp_len, (256-block_offset)));
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);
            int num_byte_to_write = 256 - block_offset;
            block_wrote += 1;
            curr_addr += num_byte_to_write;
            buffer_offset =0;
            num_written += num_byte_to_write;
            temp_len -= num_written;
            flag = 1;
            addr+=num_byte_to_write;
           

        }
        // no blocks written, starts at the beginning of block and has to write more than 256 bytes total
        else if(block_wrote==0 && block_offset==0 && temp_len >256){
            int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);

            memcpy(mybuf + block_offset, buf + buffer_offset, 256 - block_offset);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);
            int num_byte_to_write =256;
            block_wrote += 1;
            curr_addr += num_byte_to_write;
            buffer_offset =0;
            num_written += num_byte_to_write;
            temp_len -= num_written;
            flag = 1;
            addr+=num_byte_to_write;

            
        }
        // no blocks written, starts somewhere in block, has to write a block or more
        else if (block_wrote == 0 && block_offset > 0 && temp_len >= 256 ) {
            int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);

            memcpy(mybuf + block_offset, buf + buffer_offset, 256 - block_offset);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);
            int num_byte_to_write = 256 - block_offset;
            block_wrote += 1;
            curr_addr += num_byte_to_write;
            buffer_offset = num_byte_to_write;
 
            num_written += num_byte_to_write;
            temp_len -= num_written;
            flag = 1;
            addr+=num_byte_to_write;
        }
    
        // have already written bytes from a different block, more bytes
        // remaining less than 256

        else if (block_wrote > 0 && temp_len<256 && buffer_offset<256) {
            block_offset =0;
            int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            buffer_offset=num_written;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);


            memcpy(mybuf + block_offset, buf + buffer_offset, temp_len);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);
            int num_byte_to_write = temp_len;
           
            buffer_offset = num_written%256;
          
            flag = 1;
            temp_len -=num_byte_to_write;
            num_written+=num_byte_to_write;
            addr+=num_byte_to_write;
            curr_addr+=num_byte_to_write;
            block_wrote+=1;

        }
        // writing a full block after the original block has been written
        else if (block_wrote >0  && temp_len >=256 && addr%256==0 ){
            block_offset =0 ;
            int disk_num = addr / JBOD_DISK_SIZE;
            int block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
            //buffer_offset = num_written;
            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
            jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, block_num), mybuf);

            memcpy(mybuf + block_offset, buf + buffer_offset, 256);

            jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
            jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

            jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);

           int num_byte_to_write =256;

            block_wrote+=1;
            buffer_offset = 0;

            flag = 1;
            temp_len -=num_byte_to_write;
            curr_addr+=num_byte_to_write;
            num_written+=num_byte_to_write;
            addr+=num_byte_to_write;
            block_wrote+=1;
        }

        

        // case to pass invalid parameters test of addr 104860 and len 16
        else if (flag == 0) {
            break;
        }
    }

    return len;
}
