extern int F54_evb_FullRawCap(struct i2c_client *client, int mode, char *buf);
extern unsigned char F54_evb_TxToTxReport(struct i2c_client *client);
extern unsigned char F54_evb_TxToGndReport(struct i2c_client *client);
extern unsigned char F54_evb_RxToRxReport(struct i2c_client *client);
extern unsigned char F54_evb_HighResistance(struct i2c_client *client, char *buf);
extern unsigned char F54_evb_RxOpenReport(struct i2c_client *client);
extern unsigned char F54_evb_TxOpenReport(struct i2c_client *client);
