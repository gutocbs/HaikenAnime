#include "formatapalavras.h"

FormataPalavras::FormataPalavras(QObject *parent) : QObject(parent)
{

}

//Vamos testar todas as combinações possíveis. Se for igual em alguma, retornamos true.
//se não for igual em nenhuma, é false.
bool FormataPalavras::fcomparaNomes(QString rnome1, QString rnome2)
{
    try {
        if(rnome1.isEmpty() || rnome2.isEmpty())
            return false;

        if(rnome1.compare(rnome2, Qt::CaseInsensitive) == 0)
            return true;

        if(fremovePontuacao(rnome1).compare(fremovePontuacao(rnome2)) == 0)
            return true;

        if(fremoveSeason(rnome1).compare(fremoveSeason(rnome2)) == 0)
            return true;

        if(fremoveCaracteresEspeciais(rnome1).compare(fremoveCaracteresEspeciais(rnome2)) == 0)
            return true;

        if(fmudaCaracteresEspeciais(rnome1).compare(fmudaCaracteresEspeciais(rnome2)) == 0)
            return true;

        if(fmudaNumeracaoArabePraRomana(rnome1).compare(fmudaNumeracaoArabePraRomana(rnome2)) == 0)
            return true;

        if(fremoveNumeracao(rnome1).compare(fremoveNumeracao(rnome2)) == 0)
            return true;

        if(fmudaNumeracaoRomanaPraArabe(rnome1).compare(fmudaNumeracaoRomanaPraArabe(rnome2)) == 0)
            return true;

        if(fmudaNumeracaoRomanaPraSX(rnome1).compare(fmudaNumeracaoRomanaPraSX(rnome2)) == 0)
            return true;

        if(fmudaNumeracaoRomanaPraSeason(rnome1).compare(fmudaNumeracaoRomanaPraSeason(rnome2)) == 0)
            return true;

        if(fremoveNumeracaoRomana(rnome1).compare(fremoveNumeracaoRomana(rnome2)) == 0)
            return true;

        if(fremoveCaracteresExtras(rnome1).compare(fremoveCaracteresExtras(rnome2)) == 0)
            return true;

        if(fremoveTV(rnome1).compare(fremoveTV(rnome2)) == 0)
            return true;

        if(rnome1.contains("OVA") || rnome1.contains("Special") || rnome2.contains("OVA") || rnome2.contains("Special")){
            if(fmudaOVAPraSpecials(rnome1).compare(fmudaOVAPraSpecials(rnome2)) == 0)
                return true;
            if(fmudaSpecialsPraOVA(rnome1).compare(fmudaSpecialsPraOVA(rnome2)) == 0)
                return true;
            if(fmudaOVAPraSpecial(rnome1).compare(fmudaOVAPraSpecial(rnome2)) == 0)
                return true;
            if(fmudaSpecialPraOVA(rnome1).compare(fmudaSpecialPraOVA(rnome2)) == 0)
                return true;
        }

        if(fremoveOVAEspecial(rnome1).compare(fremoveOVAEspecial(rnome2)) == 0)
            return true;

        if(fremoveTudo(rnome1).compare(fremoveTudo(rnome2)) == 0)
            return true;

    }  catch (QString error) {
        qCritical() << error;
    }

    return false;
}

QString FormataPalavras::fremoveTudo(QString rpalavra)
{
    rpalavra = fremovePontuacao(rpalavra);
    rpalavra = fremoveCaracteresEspeciais(rpalavra);
    rpalavra = fremoveSeason(rpalavra);
    rpalavra = fremoveNumeracao(rpalavra);
    rpalavra = fremoveNumeracaoRomana(rpalavra);
    rpalavra = fremoveTV(rpalavra);
    rpalavra = fremoveCaracteresExtras(rpalavra);
    rpalavra = fremoveOVAEspecial(rpalavra);
    return rpalavra;
}

QString FormataPalavras::fremovePontuacao(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("...");
    rpalavra.remove(".");
    rpalavra.remove(",");
    rpalavra.remove("?");
    rpalavra.remove("!");
    rpalavra.remove(";");
    rpalavra.remove("¿");
    rpalavra.remove("¡");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveCaracteresEspeciais(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("☆", " ");
    rpalavra.replace("△", " ");
    rpalavra.replace("♥", " ");
    rpalavra.replace("○", " ");
    rpalavra.replace("…", " ");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaCaracteresEspeciais(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("☆", "-");
    rpalavra.replace("△", "-");
    rpalavra.replace("♥", "-");
    rpalavra.replace("○", "-");
    rpalavra.replace("…", "-");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveSeason(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    QString rpalavraTemp;// = rpalavra;
    for(int i = 0; i < 10; i++){
        rpalavraTemp = rpalavra;
        rpalavraTemp.replace(QString::number(i),"x");
        if(rpalavraTemp.contains("season x"))
            rpalavra.replace("season " + QString::number(i), QString::number(i));
        else if(rpalavraTemp.contains("seasonx"))
                rpalavra.replace("season" + QString::number(i), QString::number(i));
        else if(rpalavraTemp.contains("sx"))
                rpalavra.replace("s" + QString::number(i), QString::number(i));
        else if(rpalavraTemp.contains("xst season") || rpalavraTemp.contains("xnd season") ||
                rpalavraTemp.contains("xrd season") || rpalavraTemp.contains("xth season")){
            rpalavra.remove("st");
            rpalavra.remove("nd");
            rpalavra.remove("rd");
            rpalavra.remove("th");
            rpalavra.replace(QString::number(i) + " season", QString::number(i));
        }
    }
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaNumeracaoArabePraRomana(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace(" 1", " i");
    rpalavra.replace(" 2", " ii");
    rpalavra.replace(" 3", " iii");
    rpalavra.replace(" 4", " iv");
    rpalavra.replace(" 5", " v");
    rpalavra.replace(" 6", " vi");
    rpalavra.replace(" 7", " vii");
    rpalavra.replace(" 8", " viii");
    rpalavra.replace(" 9", " ix");
    rpalavra.replace(" 10", " x");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveNumeracao(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("1");
    rpalavra.remove("2");
    rpalavra.remove("3");
    rpalavra.remove("4");
    rpalavra.remove("5");
    rpalavra.remove("6");
    rpalavra.remove("7");
    rpalavra.remove("8");
    rpalavra.remove("9");
    rpalavra.remove("10");
    rpalavra = rpalavra.simplified();
    return rpalavra;

}

QString FormataPalavras::fmudaNumeracaoRomanaPraArabe(QString rpalavra)
{
    //Os números com mais caracteres tem que ser testados antes, ou vai ficar 1X, 1V, 2I
    rpalavra = rpalavra.toLower();
    rpalavra.replace(" viii", " 8");
    rpalavra.replace(" iii", " 3");
    rpalavra.replace(" vii", " 7");
    rpalavra.replace(" iv", " 4");
    rpalavra.replace(" ii", " 2");
    rpalavra.replace(" vi", " 6");
    rpalavra.replace(" ix", " 9");
    rpalavra.replace(" v", " 5");
    rpalavra.replace(" x", " 10");
    rpalavra.replace(" i", " 1");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaNumeracaoRomanaPraSX(QString rpalavra)
{
    //A ORDEM IMPORTA MUITO
    rpalavra = rpalavra.toLower();
    rpalavra.replace(" viii", " s8");
    rpalavra.replace(" iii", " s3");
    rpalavra.replace(" vii", " s7");
    rpalavra.replace(" ix", " s9");
    rpalavra.replace(" ii", " s2");
    rpalavra.replace(" vi", " s6");
    rpalavra.replace(" iv", " s4");
    rpalavra.replace(" v", " s5");
    rpalavra.replace(" x", " s10");
    rpalavra.replace(" i", " s1");
    rpalavra = rpalavra.simplified();

    return rpalavra;
}

QString FormataPalavras::fmudaNumeracaoRomanaPraSeason(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace(" viii", " season 8");
    rpalavra.replace(" iii", " season 3");
    rpalavra.replace(" vii", " season 7");
    rpalavra.replace(" ii", " season 2");
    rpalavra.replace(" iv", " season 4");
    rpalavra.replace(" vi", " season 6");
    rpalavra.replace(" ix", " season 9");
    rpalavra.replace(" v", " season 5");
    rpalavra.replace(" x", " season 10");
    rpalavra.replace(" i", " season 1");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveNumeracaoRomana(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove(" viii");
    rpalavra.remove(" iii");
    rpalavra.remove(" vii");
    rpalavra.remove(" ii");
    rpalavra.remove(" iv");
    rpalavra.remove(" ix");
    rpalavra.remove(" vi");
    rpalavra.remove(" v");
    rpalavra.remove(" x");
    rpalavra.remove(" i");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveCaracteresExtras(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("@", " ");
    rpalavra.replace("#", " ");
    rpalavra.replace("$", " ");
    rpalavra.replace("%", " ");
    rpalavra.replace("&", " ");
    rpalavra.replace("*", " ");
    rpalavra.replace("(", " ");
    rpalavra.replace(")", " ");
    rpalavra.replace(">", " ");
    rpalavra.replace("<", " ");
    rpalavra.replace("/", " ");
    rpalavra.replace("\\", " ");
    rpalavra.replace("+", " ");
    rpalavra.replace("-", " ");
    rpalavra.replace("_", " ");
    rpalavra.replace("=", " ");
    rpalavra.replace(":", " ");
    rpalavra.replace("|", " ");
    rpalavra = rpalavra.simplified();
    return rpalavra;

}

QString FormataPalavras::fmudaOVAPraSpecials(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("ova", "specials");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaSpecialsPraOVA(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("specials", "ova");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaOVAPraSpecial(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("ova", "special");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fmudaSpecialPraOVA(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("special", "ova");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString FormataPalavras::fremoveOVAEspecial(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("specials");
    rpalavra.remove("special");
    rpalavra.remove("ova");
    return rpalavra;
}

QString FormataPalavras::fremoveTV(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("(tv)");
    return rpalavra;
}
