#include "mediacomparer.h"

MediaComparer::MediaComparer(QObject *parent) : QObject(parent)
{

}

bool MediaComparer::compareEpisodeNumber(Media *media, int episode)
{
    if(episode == media->vnumEpisodiosAssistidos+1 || episode == MediaUtil::getNumberOfWatchedEpisodes(media)+1)
        return true;
    return false;
}

bool MediaComparer::isSeasonal(QString listName)
{
    if(listName.contains("Fall", Qt::CaseInsensitive) || listName.contains("Winter", Qt::CaseInsensitive)
            || listName.contains("Summer", Qt::CaseInsensitive) || listName.contains("Spring", Qt::CaseInsensitive))
        return true;
    return false;
}

//Vamos testar todas as combinações possíveis. Se for igual em alguma, retornamos true.
//se não for igual em nenhuma, é false.
bool MediaComparer::compareName(QString rnome1, QString rnome2)
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

//        if(fremoveCaracteresEspeciais(rnome1).compare(fremoveCaracteresEspeciais(rnome2)) == 0)
//            return true;

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

QString MediaComparer::fremoveTudo(QString rpalavra)
{
    rpalavra = fremovePontuacao(rpalavra);
//    rpalavra = fremoveCaracteresEspeciais(rpalavra);
    rpalavra = fremoveSeason(rpalavra);
    rpalavra = fremoveNumeracao(rpalavra);
    rpalavra = fremoveNumeracaoRomana(rpalavra);
    rpalavra = fremoveTV(rpalavra);
    rpalavra = fremoveCaracteresExtras(rpalavra);
    rpalavra = fremoveOVAEspecial(rpalavra);
    return rpalavra;
}

QString MediaComparer::fremovePontuacao(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove(QRegExp("[^\\w\\s]"));
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fremoveCaracteresEspeciais(QString rpalavra)
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

QString MediaComparer::fmudaCaracteresEspeciais(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace(QRegExp("[^\\x00-\\x7F]+"), "-");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fremoveSeason(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    if(rpalavra.contains(QRegExp("((season)+\\s*\\d|(s\\d))|\\d\\w{2}\\s(season)")))
        rpalavra.remove(QRegExp("(st|nd|rd|th)\\sseason|season\\s*|s(?![a-z])"));
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fmudaNumeracaoArabePraRomana(QString rpalavra)
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

QString MediaComparer::fremoveNumeracao(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove(QRegExp("\\d"));
    rpalavra = rpalavra.simplified();
    return rpalavra;

}

QString MediaComparer::fmudaNumeracaoRomanaPraArabe(QString rpalavra)
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

QString MediaComparer::fmudaNumeracaoRomanaPraSX(QString rpalavra)
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

QString MediaComparer::fmudaNumeracaoRomanaPraSeason(QString rpalavra)
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

QString MediaComparer::fremoveNumeracaoRomana(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove(QRegExp("i*v*i*x*i*"));
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fremoveCaracteresExtras(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace(QRegExp("[^\\w\\d\\s]"), " ");
    rpalavra = rpalavra.simplified();
    return rpalavra;

}

QString MediaComparer::fmudaOVAPraSpecials(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("ova", "specials");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fmudaSpecialsPraOVA(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("specials", "ova");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fmudaOVAPraSpecial(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("ova", "special");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fmudaSpecialPraOVA(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.replace("special", "ova");
    rpalavra = rpalavra.simplified();
    return rpalavra;
}

QString MediaComparer::fremoveOVAEspecial(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("specials");
    rpalavra.remove("special");
    rpalavra.remove("ova");
    return rpalavra;
}

QString MediaComparer::fremoveTV(QString rpalavra)
{
    rpalavra = rpalavra.toLower();
    rpalavra.remove("(tv)");
    return rpalavra;
}
