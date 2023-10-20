SQ_DT_N_UNI = "UNSIGNED INT"
SQ_DT_N_TEXT = "TEXT"

COL_N_NO = "Number"
COL_N_SKIN_TYPE = "skin_type"
COL_N_SAMPLE_POS = "sample_pos"
COL_N_SD_LAMBDA_354 = "sd_lambda_354"
COL_N_SD_LAMBDA_376 = "sd_lambda_376"
COL_N_SD_LAMBDA_405 = "sd_lambda_405"
COL_N_SD_LAMBDA_450 = "sd_lambda_450"
COL_N_SD_LAMBDA_505 = "sd_lambda_505"
COL_N_SD_LAMBDA_560 = "sd_lambda_560"
COL_N_SD_LAMBDA_600 = "sd_lambda_600"
COL_N_SD_LAMBDA_645 = "sd_lambda_645"
COL_N_SD_LAMBDA_680 = "sd_lambda_680"
COL_N_SD_LAMBDA_705 = "sd_lambda_705"
COL_N_SD_LAMBDA_800 = "sd_lambda_800"
COL_N_SD_LAMBDA_850 = "sd_lambda_850"
COL_N_SD_LAMBDA_910 = "sd_lambda_910"
COL_N_SD_LAMBDA_930 = "sd_lambda_930"
COL_N_SD_LAMBDA_970 = "sd_lambda_970"
COL_N_SD_LAMBDA_1000 = "sd_lambda_1000"
COL_N_SD_LAMBDA_1050 = "sd_lambda_1050"
COL_N_SD_LAMBDA_1070 = "sd_lambda_1070"
COL_N_SD_LAMBDA_1200 = "sd_lambda_1200"
COL_N_SAMPLE_DATE = "sample_date"
COL_N_SAMPLE_TIME = "sample_time"

cosmetic_db_name = "cosmetic_db.sqlite"

vn_tbl_name = "volunteer"
vn_tbl_headers = {COL_N_NO: SQ_DT_N_TEXT}
vn_tbl_pks = [COL_N_NO]

full_datum_tbl_name = "datum"
full_datum_tbl_headers = {
 COL_N_NO : SQ_DT_N_TEXT,
 COL_N_SKIN_TYPE : "",
 COL_N_SAMPLE_POS : "",
 COL_N_SD_LAMBDA_354 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_376 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_405 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_450 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_505 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_560 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_600 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_645 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_680 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_705 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_800 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_850 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_910 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_930 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_970 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_1000 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_1050 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_1070 : SQ_DT_N_UNI,
 COL_N_SD_LAMBDA_1200 : SQ_DT_N_UNI,
 COL_N_SAMPLE_DATE : "",
 COL_N_SAMPLE_TIME : ""
 }
full_datum_tbl_pks = [COL_N_NO, COL_N_SAMPLE_POS]
