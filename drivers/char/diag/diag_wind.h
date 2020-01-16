//
//wangpengpeng@wind-mobi.com 20180410 add begin

//

//sunsiyuan@wind-mobi.com add at 20180514 begin
#if BUILD_WIND_FOR_FACTORY_DIAG
//sunsiyuan@wind-mobi.com add at 20180514 end

#ifndef __DIAG_WIND_H__
#define __DIAG_WIND_H__


//#define SMT_CMD_SLEEP 2



extern int wind_diag_cmd_handler(unsigned char *rx_buf, int rx_len, unsigned char *tx_buf, int *tx_len);
extern int winddiag_proc_init(void);



#endif
//
//wangpengpeng@wind-mobi.com 20180410 add end

//sunsiyuan@wind-mobi.com add at 20180514 begin
#endif //BUILD_WIND_FOR_FACTORY_DIAG endif
//sunsiyuan@wind-mobi.com add at 20180514 end


