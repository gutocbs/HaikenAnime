#ifndef JANELADECONFIG_H
#define JANELADECONFIG_H

#include <QWidget>
#include <QTextStream> //Para escrever o arquivo de configuração
#include <QFileDialog> //Pra abrir as janelas de busca de pastas
#include <QDebug>
#include <QSettings>
#include <QDesktopServices> //Abre o site do anilsit
#include <QUrl>
#include <QTableWidgetItem>
#include <QBoxLayout> //Pra poder por as caixas e tickar elas
#include <QCheckBox>
#include <QWidget>
#include <QPointer>
#include <QDesktopWidget> //Pra resolução
#include <QScreen> //Resolução também
#include <QtMath> //Também resolução

#include "filtros.h"
#include "leitorlistaanimes.h"

namespace Ui {
class janeladeconfig;
}

class janeladeconfig : public QWidget
{
    Q_OBJECT

public:
    explicit janeladeconfig(QWidget *parent = nullptr);
    ~janeladeconfig();
    QByteArray fretornaUsuario();
    QStringList fretornaDiretorios();
    QByteArray fretornaCodigoAutorizacao();
    QByteArray fretornaDeteccao();
    QByteArray fretornaBaixaQualidade();
    QByteArray fretornaTorrentEscolhido();
    QByteArray fretornaDownloadAutomatico();
    QByteArray fretornaTempoDownload();
    QByteArray fretornaPastaSalvarAnimes();
    QByteArray fretornaDownloadListasAnimes();
    QByteArray fretornaSubEscolhido();
    QByteArray fretornaQualidadeEscolhida();
    QByteArray fretornaFeedAnime();
    QByteArray fretornaFeedAnimeEspecifico();
    QByteArray fretornaFiltrosDownload();
    QStringList fretornaPlayers();
    QStringList fretornaStreamLanguages();
    QString fretornaFontes();
    void fleArquivoConfig();
    void frecebeListasAnimes(leitorlistaanimes*);
    void fgetAnimeLists();
    void flimpaFiltros();
    void fselectSubFromTorrent(const QString &id, const QString &sub);
    void fsalvaFiltros();
    void fgetStreamLanguages();
    void fsetHistorico(const QStringList &rhistorico);
    void fmudaResolucao();

    QVector<Filtros*> vfiltrosAnimes;
signals:
    void sauthcodesave();
    void ssavebutton();
private slots:
    void on_botaoSalvar_clicked();
    void on_botaoAdicionar_clicked();
    void on_botaoRemover_clicked();
    void on_botaoSelecionarPastaDownload_clicked();
    void on_botaoAutorizarUser_clicked();
    void on_botaoSalvarCodigo_clicked();
    void on_botaoWatching_clicked();
    void on_botaoFiltroAdd_clicked();
    void on_botaoCompleted_clicked();
    void on_botaoOnHold_clicked();
    void on_botaoDropped_clicked();
    void on_botaoPlanToWatch_clicked();
    void on_botaoFiltroRemove_clicked();
    void on_botaoOk_clicked();
    void fupdateFiltros();
    void fupdateTabelaFiltros();
    void on_botaoCheck_clicked();
    void on_botaoInserirAnime_clicked();
    void on_botaoRemoverAnime_clicked();
    void on_botaoEnableDisable_clicked();
    void on_botaoHistorico_clicked();
    void on_botaoprefTorrent_clicked();
    void on_botaoprefDiretorios_clicked();
    void on_botaoprefFiltros_clicked();
    void on_boxFonte_currentFontChanged(const QFont &f);
    void on_boxSizeTitulo_valueChanged(const QString &arg1);
    void on_boxSizeTituloAlternativo_valueChanged(const QString &arg1);
    void on_boxSizeStatus_valueChanged(const QString &arg1);
    void on_boxSizeInfo_valueChanged(const QString &arg1);
    void on_boxSizeAvisos_valueChanged(const QString &arg1);
    void on_botaoprefFontes_clicked();

private:
    Ui::janeladeconfig *ui;

    QString vuser;
    QStringList vdirectory;
    QString vauthorization;
    QString vdetection;
    QString vtorrentSoftware;
    QString vsaveFolder;
    QString vdownloadListas;
    QString vsub;
    QString vquality;
    QString vtorrentFeed;
    QString vanimeTorrentFeed;
    QString fontName;
    QString fontSizeTitulo;
    QString fontSizeTituloAlternativo;
    QString fontSizeStatus;
    QString fontSizeInfo;
    QString fontSizeMensagem;
    QStringList vPlayers;
    QStringList vStreamLanguages;
    QStringList vhistorico;
    bool vautoDownload;
    bool vlowQuality;
    QStringList vIdNovoFiltro;

    QVector<anime*> vlistaAtual;
    leitorlistaanimes *cleitor;
};

#endif // JANELADECONFIG_H
