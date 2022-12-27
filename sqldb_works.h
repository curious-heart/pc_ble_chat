#ifndef SQLDB_WORKS_H
#define SQLDB_WORKS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QList>
#include <QMap>
#include <QFile>
#include <QThread>

#include "diy_common_tool.h"
/*
 * local db: use sqlite to store data into a local db file.
 * remote db: use mysql server.
*/
class SkinDatabase:public QObject
{
    Q_OBJECT

private:
    setting_rdb_info_t *m_remote_db_info = nullptr;

    QString m_local_db_pth_str, m_local_csv_pth_str;
    QString m_local_db_name_str, m_local_csv_name_str;
    bool m_local_db_ready = false, m_local_csv_ready = false;

public:
    typedef enum _db_ind_t
    {
        DB_NONE = 0,
        DB_REMOTE = 0x01,
        DB_SAFE_LDB = 0x02,
        DB_ALL = 0x03,
    }db_ind_t;

    typedef struct
    {
        quint32 lambda;
        quint64 data;
    }db_lambda_data_s_t;
    typedef struct
    {
        QString expt_id;
        QString expt_desc;
        QString expt_date;
        QString expt_time;
        QString obj_id;
        QString skin_type;
        QString obj_desc;
        QString dev_id;
        QString dev_addr;
        QString dev_desc;
        QString pos;
        QList<db_lambda_data_s_t> lambda_data;
        QString rec_date;
        QString rec_time;
        bool expt_changed;
        bool obj_changed;
        bool dev_changed;
    }db_info_intf_t;

    typedef enum
    {
        LOCAL,
        REMOTE,
    }db_pos_t;
    typedef enum
    {
        DB_CSV,
        DB_SQLITE,
        DB_MYSQL,
    }db_type_t;

    typedef enum
    {
        RDB_ST_UNKNOWN = -1,
        RDB_ST_OK,
        RDB_ST_ERR,
    }rdb_state_t;

    typedef struct
    {
        QString tbl_name;
        int rec_cnt, rec_succ, rec_fail, rec_part_succ;
    }tbl_rec_op_result_t;

    typedef struct
    {
        QString tbl_name;
        QString tbl_cols;
        QString primary_id_str;
    }tbl_name_cols_map_t;
private:
    db_info_intf_t m_intf;
    QString m_safe_ldb_dir_str = "";
    QString m_safe_ldb_file_str = "";
    bool prepare_local_db();

    QFile m_local_csv_f;
    bool prepare_local_csv();
    bool write_local_csv(db_info_intf_t &intf);
    rdb_state_t m_rdb_st = RDB_ST_UNKNOWN;

public:
    SkinDatabase(setting_rdb_info_t * rdb_info = nullptr);
    ~SkinDatabase();

    void set_remote_db_info(setting_rdb_info_t * db_info);
    void set_safe_ldb_for_rdb_fpn(QString safe_ldb_dir, QString safe_ldb_file);
    void set_local_store_pth_str(QString db, QString csv);
    bool store_these_info(db_info_intf_t &info);
    static bool create_tbls_and_views(QSqlDatabase &qdb,
                                      db_pos_t db_pos, db_type_t db_type);
    static bool write_remote_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                                db_type_t db_type, bool use_safe_ldb,
                                QString safe_ldb_pth_str,
                                QString safe_ldb_name_str,
                                QString safe_ldb_conn_name_str,
                                bool &safe_ldb_ready, db_ind_t &ret_ind);
    static bool write_local_db(QSqlDatabase &qdb, db_info_intf_t &intf, db_type_t db_type);
    /*
     * This write_db may be called from multi thread simultaneous.
     * When the db_pos is REMOTE, the parameters with default value should be assigned
     * proper value.
    */
    static bool write_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                         db_pos_t db_pos, db_type_t db_type, bool use_safe_ldb = false,
                         QString safe_ldb_pth_str = "", QString safe_ldb_name_str = "",
                         QString safe_ldb_conn_name_str = "",
                         bool *safe_ldb_ready = nullptr, db_ind_t *ret_ind = nullptr);
    static bool prepare_safe_ldb(QString pth, QString name, QString tbl_name,
                                    QString safe_ldb_conn_name_str);
    static bool db_ins_err_process(QSqlDatabase &qdb, db_info_intf_t &intf,
                                   QSqlError &cur_sql_err, QString &cur_cmd,
                                   QString &tbl_name, db_type_t db_type);
    void close_dbs(db_ind_t db_ind);
    static bool safe_ldb_to_remote_db(QSqlDatabase &safe_ldb, QSqlDatabase &rdb,
                                      QList<SkinDatabase::tbl_rec_op_result_t>& op_result);

    rdb_state_t remote_db_st();
    void prepare_remote_db();
    void upload_safe_ldb(QString safe_ldb_fpn);

signals:
    /*sent to remote_worker.*/
    bool prepare_rdb_sig(setting_rdb_info_t rdb_info,
                         QString safe_ldb_dir_str, QString safe_ldb_file_str);
    bool write_rdb_sig(SkinDatabase::db_info_intf_t intf, setting_rdb_info_t rdb_info,
                       QString safe_ldb_dir_str, QString safe_ldb_file_str);
    bool close_rdb_sig(SkinDatabase::db_ind_t db_ind);
    void upload_safe_ldb_sig(QString safe_ldb_fpn);

    /*sent to chat*/
    void rdb_state_upd_sig(SkinDatabase::rdb_state_t rdb_st);
    void upload_safe_ldb_end_sig(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                 bool result_ret);
    void rdb_write_start_sig();
    void rdb_write_done_sig(SkinDatabase::db_ind_t write_ind, bool ret);
public slots:
    /*handle signal from remote worker.*/
    void rdb_prepare_ret_handler(bool rdb_p);
    void upload_safe_ldb_done_handler(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                      bool result_ret);
    void remote_db_write_done_handler(SkinDatabase::db_ind_t write_ind, bool ret);
private:
    QThread m_rdb_thread;
    static void get_sql_select_helper(QMap<QString, void*> &pointer,
                                  SkinDatabase::db_info_intf_t &intf,
                                  QString tbl_name = "");
    static const tbl_name_cols_map_t* get_tbl_name_cols_map(QString tbl_name,
                                                            int * cnt = nullptr);
    static QString fill_intf_from_sql_query_record(QString &tbl_name,
                                                QStringList &col_list,
                                                QSqlQuery &db_q,
                                                QMap<QString, void*> &helper,
                                                const QString &primary_key_str);
    static void merge_two_db_intf(db_info_intf_t &drawer, db_info_intf_t &sender,
                                  QString tbl_name = "");
};

/*sqlerr must be of type QSqlError*/
#define SQL_LAST_ERR_STR(sqlerr)\
            (QString("err_txt:") + (sqlerr).text() + "\n"\
             + "native_code:" + (sqlerr).nativeErrorCode() + "; "\
             + "err type:" + QString("%1").arg((int)((sqlerr).type())))

void remove_qt_sqldb_conn(QString conn_name);
#endif // SQLDB_WORKS_H
