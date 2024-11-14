/*********************************************
 * Concert Model
 * Autor: Elisama Oliveira
 * Data de criação: 14-04-2021
 * Problem - Problema de Corte de Estoque com
             Data de Entrega e
             Setup de Padrao
 *********************************************/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <ilcplex/ilocplex.h>
#include <algorithm>
#include <map>
#include <time.h>

using namespace std;
#define EPSILON 10e-6
#define M 10e20
#define RC_EPS 1.0e-0
#define tab '\t'


ILOSTLBEGIN

typedef IloArray<IloRangeArray> IloRangeMatrix2;
typedef IloArray<IloRangeMatrix2> IloRangeMatrix3;

static void report1 (IloCplex& cutSolver,
                     IloNum& periodos,
                     IloArray<IloNumVarArray> Cut,
                     IloArray<IloRangeArray> Fill);

static void report2 (IloAlgorithm& patSolver,
                     IloNum& periodos,
                     IloArray<IloNumVarArray> Use,
                     IloObjective obj);

static void report3 (IloCplex& cutSolver,
                     IloNum& periodos,
                     IloNum& numItens,
                     IloNumVarArray Atraso,
                     IloNumVarArray Adiantamento,
                     IloNumVarArray TempoPreparo,
                     IloArray<IloNumVarArray> Cut);

/** HEURISTICA LOCAL BRANCHING **/

static void  Relax_and_Fix(IloCplex& cplex,
                           IloNum& periodos,
                           IloNumArray& AtrasoRel,
                           IloNumArray& AdiantamentoRel,
                           IloNumArray& TempoPreparoRel,
                           IloArray<IloNumVarArray> Cut,
                           IloArray<IloNumArray> sol,
                           IloArray<IloNumArray> SetupRel,
                           IloInt objFO);

static void Local_Branching(IloAlgorithm& cplexLB,
                            IloNum& periodos,
                            IloNumArray AtrasoLB,
                            IloNumArray AdiantamentoLB,
                            IloNumArray TempoPreparoLB,
                            IloArray<IloNumVarArray> Cut,
                            IloArray<IloNumArray> sol,
                            IloNum& ObjFOLB);

static void VNDB(IloAlgorithm& cplexVNDB,
                 IloNum& periodos,
                 IloNumArray AtrasoVNDB,
                 IloNumArray AdiantamentoVNDB,
                 IloNumArray TempoPreparoVNDB,
                 IloArray<IloNumVarArray> Cut,
                 IloArray<IloNumArray> sol,
                 IloNum& ObjFOVNDB);

static void Arredondamento (IloCplex& cplex,
                            IloNum& periodos,
                            IloNumArray& AtrasoRel,
                            IloNumArray& AdiantamentoRel,
                            IloNumArray& TempoPreparoRel,
                            IloArray<IloNumVarArray> Cut,
                            IloArray<IloNumArray> sol,
                            IloInt objFO);

/// VALOR MAXIMO DE ENTRE DOIS VALORES
int max(int a, int b)
{
    int x = a > b ? a:b;
    return (x);
}
int max(int a, int b);






///========================================================================================================///
///                                         PROGRAMA PRINCIPAL                                             ///
///========================================================================================================///


/* Função principal */
void  principal(char instancia[]){

     IloEnv env;

    try{

        IloModel cutOpt (env);


        IloTimer crono(env);  /// Variável para coletar o tempo
        crono.start();

        IloTimer crono2(env);  /// Variável para coletar o tempo
        crono2.start();

        IloTimer crono3(env);  /// Variável para coletar o tempo
        crono3.start();

        IloTimer cronoNo(env);  /// Variável para coletar o tempo
        cronoNo.start();
/**********************************************************************************************************************************************/
/****************************                            LEITURA DE DADOS                          ********************************************/
/**********************************************************************************************************************************************/

      IloInt i, j, t;
        IloNum CompObjeto, periodos, numItens, CustoSetup;

        ifstream entrada(instancia);
        entrada  >> CompObjeto;
        entrada  >> numItens;
        entrada  >> periodos;


        IloNumArray size(env,numItens);                 /** comprimento dos itens **/
        for(int i=0; i<numItens; i++)
            entrada >> size[i];


          IloArray<IloNumArray> demanda(env,periodos);
        for(int t=0; t<periodos; t++){
            demanda[t] = IloNumArray(env, numItens);
            for(int i=0; i<numItens; i++)
                entrada >> demanda[t][i];
        }

          IloNumArray data(env,periodos);                 /** comprimento dos itens **/
        for(int t=0; t<periodos; t++)
            entrada >> data[t];
/*
          IloNumArray alfa(env,periodos);
        for(int t=0; t<periodos; t++)
            entrada >> alfa[t];

          IloNumArray beta(env,periodos);
        for(int t=0; t<periodos; t++)
            entrada >> beta[t];
/*
          IloNumArray f(env,periodos);
        for(int t=0; t<periodos; t++)
            entrada >> f[t];

*/
/**********************************************************************************************************************************************/
/**********************                     DECLARANDO VARIAVEIS DO MODELO                                *************************************/
/**********************************************************************************************************************************************/



       int numMaxPadroes = 1000;



       /******************* CORTE DE ESTOQUE ********************/
       IloArray<IloNumVarArray> Cut(env, periodos);
         for(IloInt t=0; t<periodos; t++)
            Cut[t] = IloNumVarArray(env);


       /******************* CORTE DE ESTOQUE ********************/
       IloArray<IloNumVarArray> Setup_Troca(env, periodos);
         for(IloInt t=0; t<periodos; t++)
            Setup_Troca[t] = IloNumVarArray(env);


        /*********************  ESTOQUE  *****************************/
        IloArray<IloNumVarArray> Estoque_Itens(env, periodos);   /** perda do padrao de corte **/
         for(IloInt t=0; t<periodos; t++){
            Estoque_Itens[t] = IloNumVarArray(env, numItens, 0.0,  IloInfinity, ILOFLOAT);
         }

        /********************* VALORES DE TEMPO *****************************/
        IloNumVarArray Atraso(env,periodos,0.0, IloInfinity, ILOFLOAT);
        IloNumVarArray Adiantamento(env,periodos,0.0, IloInfinity, ILOFLOAT);
        IloNumVarArray TempoPreparo(env,periodos, 0.0, IloInfinity, ILOFLOAT);



/***************************************************************/
/**************     PARAMETROS DO MODELO     *******************/
/***************************************************************/

     /*** CUSTO DE CORTAR UM PADRAO DE CORTE ***/
      IloNumArray c(env, periodos);


      /*** TEMPO DE CORTAR UM PADRAO DE CORTE ***/
      IloNumArray p(env, periodos);


     /*** TEMPO DE PREPARAÇÃO DA MAQUINA PARA CORTAR UM PADRAO DE CORTE ***/
      IloNumArray r(env, periodos);


      /*** CUSTO DE SETUP DE UM PADRAO DE CORTE ***/
       IloNumArray f(env, periodos);


     /*** CUSTO PAGO POR ATRASAR ***/
         IloNumArray alfa(env, periodos);


     /*** CUSTO PAGO POR ADIANTAR ***/
        IloNumArray beta(env, periodos);


     /*** VALOR B-GRANDE ***/
         IloNum B;
/**********************************************************************************************************************************************/
/****************************                  RESTRIÇÕES DO MODELO COM ILORANGE                        ***************************************/
/**********************************************************************************************************************************************/


     IloArray<IloRangeArray>Fill(env,periodos);
         for (IloInt t = 0; t < periodos ; t++){
          Fill[t] = IloRangeArray(env, numItens);
             for(IloInt i=0; i<numItens; i++){
              if(t==0)
               Fill[t][i] = IloRange(env, 0.0,  -demanda[t][i] - Estoque_Itens[t][i], 0.0);
              else
                Fill[t][i] = IloRange(env, 0.0, Estoque_Itens[t-1][i] - demanda[t][i] - Estoque_Itens[t][i], 0.0);
cutOpt.add(Fill[t][i]);
           }
        }



///==========================================///
///     CONTADOR DE TEMPO DE PERPARO         ///
///==========================================///

       IloRangeArray RangeTempo(env, periodos);
        for (IloInt t = 0; t < periodos ; t++){
            RangeTempo[t] = IloRange(env, 0.0, - TempoPreparo[t], 0.0);
      cutOpt.add(RangeTempo[t]);
         }





    IloRangeMatrix2 FillSetup(env, periodos);
     for (IloInt t = 0; t<periodos; t++){
      FillSetup[t] = IloRangeArray(env,numMaxPadroes);
       for(IloInt j=0; j<numMaxPadroes; j++){
         FillSetup[t][j] = IloRange(env, - IloInfinity, 0.0 );
      cutOpt.add(FillSetup[t][j]);
       }
     }




 IloNumVarArray Tinicio(env, periodos, 0.0, IloInfinity, ILOFLOAT);


        for(IloInt t=0; t<periodos; t++){
            if(t==0)
            cutOpt.add(Tinicio[t] >= 0.0) ;
            else
            cutOpt.add( Tinicio[t] >= data[t-1] + Atraso[t-1] - Adiantamento[t-1]);
        }



      IloRangeArray RangeAtrasoAdiantado(env, periodos);
           for (IloInt t = 0; t<periodos; t++){
           RangeAtrasoAdiantado[t] = IloRange(env,  0.0, Adiantamento[t] - Atraso[t] + Tinicio[t] + TempoPreparo[t]  - data[t], 0.0 );
         cutOpt.add(RangeAtrasoAdiantado[t]);
        }


///==========================================================================================================================================///
///                                               DAR NOMES AS VARIAVEIS                                                                     ///
///==========================================================================================================================================///
        char name[255];

        for(IloInt t=0; t<periodos; t++){

            sprintf(name, " Tempo_Preparo[%d]", t+1 );
            TempoPreparo[t].setName(name);

            sprintf(name, " Rest__Preparo[%d]", t+1 );
            RangeTempo[t].setName(name);

            sprintf(name, " Rest_Atraso_Adiantado[%d]", t+1 );
            RangeAtrasoAdiantado[t].setName(name);

            sprintf(name, " TInicio[%d]", t+1 );
            Tinicio[t].setName(name);

            sprintf(name, " Atraso[%d]", t+1 );
            Atraso[t].setName(name);

            sprintf(name, " Adiantamento[%d]", t+1 );
            Adiantamento[t].setName(name);

            for(IloInt i=0; i<numItens; i++){
                sprintf(name, " Rest_Demanda[%d][%d]", t+1, i+1  );
                Fill[t][i].setName(name);
            }

            for(IloInt j=0; j<numMaxPadroes; j++){
             sprintf(name, " Fill_Setup[%d][%d]", t+1, j+1  );
                FillSetup[t][j].setName(name);
            }


        }



        /**********************************************************************************************************************************************/
        /*****************************************                 FUNÇÃO OBJETIVO DO MODELO                                ***************************/
        /**********************************************************************************************************************************************/

        B = 1000;

        for(IloInt t=0; t<periodos; t++)
        {
           alfa[t] = 100;
           beta[t] = 100;

            c[t] = 1000;
            f[t] = 100;

            p[t] = 1;
            r[t] = 1;
        }
    //cout << " alfa = " << alfa << endl;

/****************** CONFIGURAÇÃO ******************/
  //  20   25   12  215 1
  //  20   18  723  361 3
  //  18   16  782  436 1
  //  11    2  546  660 4
/**************************************************/

        IloExpr Objetos(env);
        for(IloInt t=0; t<periodos; t++)
        {

            Objetos += alfa[t]*Atraso[t];
            Objetos += beta[t]*Adiantamento[t];
        }

        IloObjective RollsUsed = IloAdd(cutOpt, IloMinimize(env, Objetos));





        /**********************************************************************************************************************************************/
///                                                 ADICIONA COLUNAS INICIAIS : COLUNAS HOMOGENEAS                                           ///
        /**********************************************************************************************************************************************/


      IloArray<IloNumArray> PadraoCorte1(env, numItens);
        for(IloInt j=0; j<numItens; j++)
            PadraoCorte1[j] = IloNumArray(env, numItens);

        for(IloInt j=0; j<numItens; j++){
            for(IloInt i=0; i<numItens; i++){
                if(i==j)
//                  PadraoCorte1[j][i] = 1;
                    PadraoCorte1[j][i] = (int(CompObjeto / size[i]));
                else
                    PadraoCorte1[j][i] = 0;
             }
        }

/*
         IloArray<IloNumArray> PadraoCorte1(env, 2*numItens);
        for(IloInt j=0; j<2*numItens; j++)
            PadraoCorte1[j] = IloNumArray(env, numItens);

        for(IloInt j=0; j<numItens; j++){
            for(IloInt i=0; i<numItens; i++){
                if(i==j)
                    PadraoCorte1[j][i] = (int(CompObjeto / size[i]));
                else
                    PadraoCorte1[j][i] = 0;
             }
        }

        for(IloInt j=numItens; j<2*numItens; j++){
            for(IloInt i=0; i<numItens; i++){
                if(j==numItens+i){
                    PadraoCorte1[j][i] = 1;
                }
                else{
                    PadraoCorte1[j][i] = 0;
              }
            }
        }

   //     for(IloInt j=0; j<2*numItens; j++)
   //    cout << " PADROA HOMOGENEO " << PadraoCorte1[j] << endl;



/**************************************************************************************************/
///     ADICIONA COLUNAS INICIAIS : COLUNAS HOMOGENEAS                                           ///
/**************************************************************************************************/
      IloNum custoInicialSetup ;




    for (IloInt t = 0; t < periodos ; t++){
        for (IloInt i= 0; i<numItens; i++){

          cutOpt.add(FillSetup[t][i]);

          IloNumColumn col = RollsUsed(c[t]);
                for (IloInt j= 0; j<numItens; j++)
                col += Fill[t][i](PadraoCorte1[j][i]);
                col += FillSetup[t][i](1);
                col += RangeTempo[t](p[t]);
          Cut[t].add(IloNumVar(col, 0, IloInfinity));



        IloNumColumn col2 = RollsUsed(f[t]);
             col2 += RangeTempo[t](r[t]);
             col2 += FillSetup[t][i](-B);
          Setup_Troca[t].add(IloNumVar(col2, 0, 1));
            }
       }





        IloCplex cutSolver(cutOpt);

        cutSolver.setParam( IloCplex::Param::MIP::Tolerances::MIPGap, 10e-12);
        cutSolver.setOut(env.getNullStream());
        cutSolver.setWarning(env.getNullStream());



        IloModel patGen (env);
   ///----------------------------------------///
   ///        DADOS PARA A MOCHILA            ///
   ///----------------------------------------///

        IloArray<IloNumVarArray> Use(env, periodos);
        for(IloInt t=0; t<periodos; t++)
            Use[t] = IloNumVarArray(env, numItens, 0, IloInfinity, ILOINT);


        IloArray<IloNumArray> newPatt(env, periodos);
        for(IloInt t=0; t<periodos; t++)
            newPatt[t] = IloNumArray(env, numItens);

        IloArray<IloNumArray> priceCortes(env, periodos);
        for(IloInt t=0; t<periodos; t++)
            priceCortes[t] = IloNumArray(env, numItens);

        IloNumArray priceTempo(env, periodos);


   ///----------------------------------------///
   ///      FUNÇÃO OBJETIVO DA MOCHILA        ///
   ///----------------------------------------///
        IloObjective ReducedCost = IloAdd(patGen, IloMinimize(env));

   ///----------------------------------------///
   ///     RESTRIÇÃO  DA MOCHILA        ///
   ///----------------------------------------///
        for(IloInt t=0; t<periodos; t++)
         patGen.add(IloScalProd(size, Use[t]) <= CompObjeto);

        IloCplex patSolver(patGen);

        patSolver.setParam( IloCplex::Param::MIP::Tolerances::MIPGap, 10e-100);
        patSolver.setOut(env.getNullStream());
        patSolver.setWarning(env.getNullStream());


        IloNum custoSetup;

        /**************************************************************/
        /********        PROCESSO DE GERAÇÃO DE COLUNAS      **********/
        /**************************************************************/
for(;;){
            /**************************************/
            /***   OTIMIZE OS PADROES ATUAIS    ***/
            /**************************************/

            cutSolver.solve();

            report1 (cutSolver, periodos, Cut,   Fill);


            /**************************************/
            /***   DEFINE AS VARIAVEIS DUAIS    ***/
            /**************************************/
            IloNumArray novaColuna(env,periodos, 0, IloInfinity, ILOINT);

            for(IloInt t=0; t<periodos; t++)
                novaColuna[t] = 0; // Flag para identificar se haverá uma coluna para o período t ou não

            IloInt NumNovaCol = 0; // Contador para identificar quantas novas colunas serão adicionadas

            for(IloInt t=0; t<periodos; t++){

                for(IloInt i=0; i<numItens ; i++){
                    priceCortes[t][i] =  cutSolver.getDual(Fill[t][i]);
                }

                priceTempo[t] =  cutSolver.getDual(RangeTempo[t]);

                IloExpr objExpr = ReducedCost.getExpr();///
                objExpr.clear();///

                objExpr += (c[t] + (f[t]/B));

                for(IloInt i=0 ; i<numItens; i++)
                    objExpr += -Use[t][i]*priceCortes[t][i];

                objExpr +=  -priceTempo[t]*((p[t] + (r[t]/B)));

                ReducedCost.setExpr( objExpr);//
                objExpr.end();




                /**************************************/
                /****    RESOLVE O SUBPROBLEMA     ****/
                /**************************************/

                patSolver.solve();

                report2 (patSolver,periodos, Use, ReducedCost);


                /**************************************/
                /****     CRITERIO DE PARADA       ****/
                /**************************************/
                if (patSolver.getValue(ReducedCost) >- RC_EPS){
                    //		cout <<  " ---- Nesse periodo nao existe padrao que melhore! ---- " << endl;
                   // 	cout << " PARE! Custo Reduzido = " << patSolver.getValue(ReducedCost) << endl;
                    //		cout << " Nova Coluna = " << novaColuna[t] << endl;

                    // break;

                    continue;  //Elisama Comentario: esse parte eh necessaria, pois coso contrario se add uma coluna que ja foi add
                }
                else {
                novaColuna[t] = novaColuna[t] + 1 ; // Contabiliza como 1 caso uma coluna para t seja zerada
                NumNovaCol = NumNovaCol + 1; // incrementa o número de colunas que serão adicionadas
                }


                patSolver.getValues(newPatt[t], Use[t]);


    //cout << " padrao " << Cut[t].getSize() + 1 << " eh -- " << newPatt[t] << " CUSTO DE SETUP = " << custoSetup << endl;
    }


   if(NumNovaCol == 0){  // Para a execução apenas se o número de novas colunas for zero
      break;
   }



            /**************************************/
            /***  ADD UM NOVO PADRAO DE CORTE   ***/
            /**************************************/


    for(IloInt t=0; t<periodos; t++){

       if(novaColuna[t] == 1){  //Aqui: só adiciona uma coluna para o período t se o custo reduzido foi negativo, ou seja, chegou no else da linha 521

         IloNumColumn col = RollsUsed(c[t]);
            for(IloInt i=0; i<numItens; i++)
                col += Fill[t][i](newPatt[t][i]);
                col += FillSetup[t][Cut[t].getSize()](1);
                col += RangeTempo[t](p[t]);
        Cut[t].add(IloNumVar(col, 0, IloInfinity));



         IloNumColumn col2 = RollsUsed(f[t]);
                col2 += RangeTempo[t](r[t]);
                col2 += FillSetup[t][Cut[t].getSize()](-B);
         Setup_Troca[t].add(IloNumVar(col2, 0, 1));


         } // if
      }  // for

  } // {;;}




       for(IloInt t=0; t<periodos; t++){
                   for(IloInt j=0; j<Cut[t].getSize(); j++){
                    cutOpt.add(IloConversion(env, Cut[t][j], ILOINT));
                    cutOpt.add(IloConversion(env, Setup_Troca[t][j], ILOINT));
                   }
        }


        cutSolver.setParam(IloCplex::Threads,1);
        cutSolver.setParam(IloCplex::Param::TimeLimit,1200);


        cutSolver.solve();
        cutSolver.exportModel("Model-ModelColunas2.lp");

        cutSolver.setOut(env.getNullStream());
        cutSolver.setWarning(env.getNullStream());

        IloExpr NumPadroes(env);
        IloExpr setups(env);
        IloExpr objObjetosCortados(env);
        IloExpr objFOTRgc(env);
        IloExpr objFOARgc(env);
        IloExpr objFOgc(env);
        IloExpr gap_CPLEX(env);



        for(IloInt t=0; t<periodos; t++){
            NumPadroes += Cut[t].getSize() - numItens;

            for(IloInt j=0; j<Cut[t].getSize(); j++){
                objObjetosCortados += cutSolver.getValue(Cut[t][j]);
                setups += cutSolver.getValue(Setup_Troca[t][j]);
            }

            objFOTRgc += cutSolver.getValue(Atraso[t]);
            objFOARgc += cutSolver.getValue(Adiantamento[t]);
            objFOgc += cutSolver.getValue(TempoPreparo[t]);
            // gap_CPLEX += cutSolver.
        }

        IloNumExprArg MediaNumPadroes = NumPadroes / periodos;

        IloNum OBJFOGC = cutSolver.getObjValue();



                  crono.stop();
                  double time_crono = crono.getTime() ;

     cout << " função " << OBJFOGC << endl;
     cout << " time "  << time_crono << endl;



        report3 (cutSolver, periodos,
                 numItens,
                 Atraso,
                 Adiantamento,

                 TempoPreparo,
                 Cut);







///================================================================================================================///
///                                 HEURISTICA RELAX AND FIX POR PERIODO                                           ///
///================================================================================================================///

/*
		IloCplex cutSolverRF(cutOpt);

		cutSolverRF.setParam(IloCplex::Threads,1);
		cutSolverRF.setParam(IloCplex::Param::TimeLimit,1200);
        cutSolverRF.setOut(env.getNullStream());
		cutSolverRF.setWarning(env.getNullStream());


        IloNum TotalLimiteS = 1200;
        IloNum  objFOA = IloInfinity;



        ///=====================================================================///
        /// DETERMINAR OS VALORES DO ATRASO e ADIANTAMENTO E TEMPO DE PREPARO   ///
        /// APOS O RELAX AND FIX                                                ///
        ///=====================================================================///

                IloNumArray AtrasoRel(env, periodos);
                IloNumArray AdiantamentoRel(env, periodos);


                IloArray<IloNumArray> estoqueRel(env, periodos);
                  for(IloInt t=0; t<periodos; t++)
                    estoqueRel[t] = IloNumArray(env, numItens);


        ///=======================================================================///
        ///                          SALVA A SOLUCAO                              ///
        ///=======================================================================///
                IloNumArray TempoPreparoRel(env, periodos);
                IloNumArray TempoInicioRel(env, periodos);

                IloArray<IloNumArray> sol(env, periodos);
                for(IloInt t=0; t<periodos; t++)
                    sol[t] = IloNumArray(env, Cut[t].getSize());

                  IloArray<IloNumArray> setupRel(env, periodos);
                for(IloInt t=0; t<periodos; t++)
                    setupRel[t] = IloNumArray(env, Cut[t].getSize());

        ///========================================================================///
        ///                         RELAX AND FIX POR PERIODO                      ///
        ///========================================================================///

                for(IloInt t=0; t<periodos; t++){

        ///===========================================================///
        ///          add a variavel inteira ao modelo                 ///
        ///===========================================================///


                    for(IloInt j =0; j<Cut[t].getSize(); j++){
                      cutOpt.add(IloConversion(env, Cut[t][j], ILOINT));
                      cutOpt.add(IloConversion(env, Setup_Troca[t][j], ILOBOOL));
                    }




        ///===========================================================///
        ///  remove a relaxacao (relaxa) para os periodos a frente    ///
        ///===========================================================///
                   for(IloInt t2=t+1; t2<periodos; t2++){
                     for(IloInt j =0; j<Cut[t2].getSize(); j++){
                      cutOpt.remove(IloConversion(env, Cut[t2][j], ILOINT));
                      cutOpt.remove(IloConversion(env, Setup_Troca[t2][j], ILOBOOL));
                     }
                    }

               cutSolverRF.setParam(IloCplex::TiLim, TotalLimiteS  - crono2.getTime()); ///Tempo limite de resolução
               /// RESOLVE O PROBLEMA
               cutSolverRF.solve();
               crono2.stop();


                    if(cutSolverRF.getStatus()==IloAlgorithm::Infeasible ){
                    cout << "Erro! Particao infactivel!" << endl;
                    cout << " Status " << cutSolverRF.getStatus() << endl;
                    }

        ///============================================================///
        ///                salva a parte inteira                       ///
        ///============================================================///


                   for(IloInt j =0; j<Cut[t].getSize(); j++){
                       sol[t][j] = cutSolverRF.getValue(Cut[t][j]);  /// guarda a solucao corrente
                       setupRel[t][j] = cutSolverRF.getValue(Setup_Troca[t][j]);  /// guarda a solucao corrente
                    }


                    for(IloInt i=0; i< numItens; i++){
                       estoqueRel[t][i] = cutSolverRF.getValue(Estoque_Itens[t][i]);
                    }


                    AtrasoRel[t] = cutSolverRF.getValue(Atraso[t]);
                    AdiantamentoRel[t] = cutSolverRF.getValue(Adiantamento[t]) ;
                    TempoPreparoRel[t] = cutSolverRF.getValue(TempoPreparo[t]);

        objFOA = cutSolverRF.getObjValue();

        cout<< " Função Objetivo no periodo : " <<  t + 1 <<  " eh " << objFOA  << endl;



        ///=============================================///
        /// fixa a parte inteira que foi integralizada  ///
        ///=============================================///
                   for(IloInt j =0; j<Cut[t].getSize(); j++){
                        Cut[t][j].setBounds(sol[t][j], sol[t][j]);
                        Setup_Troca[t][j].setBounds(setupRel[t][j], setupRel[t][j]);
                    }



                        Atraso[t].setBounds(AtrasoRel[t], AtrasoRel[t]);
                        Adiantamento[t].setBounds(AdiantamentoRel[t], AdiantamentoRel[t]);
                        TempoPreparo[t].setBounds(TempoPreparoRel[t], TempoPreparoRel[t]);

                        for(IloInt i=0; i<numItens; i++)
                         Estoque_Itens[t][i].setBounds(estoqueRel[t][i], estoqueRel[t][i]);

        }

        cout<< " Função Objetivo  : "  << objFOA  << endl;
        IloNum time2 = (double)crono2.getTime()  - time_crono;

        ///========================================================///
        ///                  REINICIALIZA OS BOUNDS                ///
        ///========================================================///
             for(IloInt t=0; t<periodos; t++){
                    Atraso[t].setBounds(0, IloInfinity);
                    Adiantamento[t].setBounds(0, IloInfinity);
                    Tinicio[t].setBounds(0, IloInfinity);
                    TempoPreparo[t].setBounds(0, IloInfinity);

                    for(IloInt j=0; j<Cut[t].getSize(); j++){
                        Cut[t][j].setBounds(0, IloInfinity);
                        Setup_Troca[t][j].setBounds(0, 1);
                    }
                }
*/

///======================================================================================================///
///                       HEURISTICA DE ARREDONDAMENTO PARA CIMA                                        ///
///======================================================================================================///

/*
               IloCplex cplex(cutOpt);

                cplex.setParam(IloCplex::Threads,1);
                cplex.setParam( IloCplex::Param::MIP::Tolerances::MIPGap, 10e-12);

                cplex.setOut(env.getNullStream());
                IloNum objFOA = IloInfinity;

                ///=====================================///
                /// DETERMINAR OS VALORES DO ATRASO     ///
                /// ADIANTAMENTO E TEMPO DE PREPARO     ///
                /// APOS O RELAX AND FIX                ///
                ///=====================================///

                IloNumArray AtrasoRel(env, periodos);
                IloNumArray AdiantamentoRel(env, periodos);
                IloNumArray TempoPreparoRel(env, periodos);
                IloNumArray TinicioRel(env, periodos);

        IloArray<IloNumArray> estoqueRel(env, periodos);
             for(IloInt t=0; t<periodos; t++)
                estoqueRel[t] = IloNumArray(env,numItens);

        ///==========================================================///
        ///                SALVA A SOLUCAO                           ///
        ///==========================================================///
                IloArray<IloNumArray> sol(env, periodos);
                for(IloInt t=0; t<periodos; t++)
                    sol[t] = IloNumArray(env, Cut[t].getSize());

                IloArray<IloNumArray> setupRel(env, periodos);
                for(IloInt t=0; t<periodos; t++)
                    setupRel[t] = IloNumArray(env, Cut[t].getSize());

        ///==========================================================///
        ///                ARREDONDA A SOLUCAO                           ///
        ///==========================================================///
                for(IloInt t=0; t<periodos; t++)  {
                    for(IloInt j =0; j<Cut[t].getSize(); j++){
                        sol[t][j] = ceil(cutSolver.getValue(Cut[t][j]));  /// guarda a solucao corrente
                        setupRel[t][j] = ceil(cutSolver.getValue(Setup_Troca[t][j]));  /// guarda a solucao corrente
                   }
                }


                 for(IloInt t=0; t<periodos; t++){
                      for(IloInt j =0; j<Cut[t].getSize(); j++){
                        Cut[t][j].setBounds(sol[t][j], sol[t][j]);
                        Setup_Troca[t][j].setBounds(setupRel[t][j], setupRel[t][j]);
                      }
                   }
        cplex.setParam(IloCplex::TiLim, 600  - crono2.getTime()); ///Tempo limite de resolução

                cplex.solve();


                for(IloInt t=0; t<periodos; t++)  {
                    AtrasoRel[t] = cplex.getValue(Atraso[t]);
                    AdiantamentoRel[t] = cplex.getValue(Adiantamento[t]) ;
                    TempoPreparoRel[t] = cplex.getValue(TempoPreparo[t]);
                    TinicioRel[t] = cplex.getValue(Tinicio[t]);

                    for(IloInt i=0; i<numItens; i++)
                    estoqueRel[t][i] = cplex.getValue(Estoque_Itens[t][i]);
                }


       objFOA = cplex.getObjValue();
       crono2.stop();

       cout << " Função Arredondada " << objFOA  << " -- " << (double)crono2.getTime() << endl ;
       //cout << " Cut"  << sol << endl;
       //cout << " setup "  << setupRel << endl;

   IloNum time2 = (double)crono2.getTime()  - time_crono;

    Arredondamento (cplex, periodos, AtrasoRel, AdiantamentoRel, TempoPreparoRel, Cut, sol, objFOA);



     IloExpr objObjetosCortadosar(env);
                IloExpr objFOat(env);
                IloExpr objFOad(env);
                IloExpr setupsar(env);




                for(IloInt t=0; t<periodos; t++){
                 for(IloInt j=0; j<Cut[t].getSize(); j++){
                   objObjetosCortadosar += sol[t][j] ;
                   setupsar += setupRel[t][j];
                 }

                   objFOat += AtrasoRel[t];
                   objFOad += AdiantamentoRel[t];
                 }

        ///========================================================///
        ///                  REINICIALIZA OS BOUNDS                ///
        ///========================================================///
           for(IloInt t=0; t<periodos; t++){
                    Atraso[t].setBounds(0, IloInfinity);
                    Adiantamento[t].setBounds(0, IloInfinity);
                    Tinicio[t].setBounds(0, IloInfinity);
                    TempoPreparo[t].setBounds(0, IloInfinity);

                    for(IloInt j=0; j<Cut[t].getSize(); j++){
                        Cut[t][j].setBounds(0, IloInfinity);
                        Setup_Troca[t][j].setBounds(0, 1);
                    }
                }



///====================================================================================================///
///                             HEURISTICA FIX AND OPTIMIZE                                              ///
///======================================================================================================///


/*

        IloCplex cplex2(cutOpt);
        cplex2.setOut(env.getNullStream());
		cplex2.setWarning(env.getNullStream());


		IloNum ofjfixop;

        cout<< "   "  << endl;
        cout << " -- Iniciando a Heuristica Fix and Optimize -- "  << endl;


        ///===============================================///
        ///      converte a variavel Setup em inteira       ///
        ///===============================================///
/*
          IloArray<IloExtractableArray> integraliza2(env, periodos);
            for(IloInt t =0; t<periodos; t++)
                integraliza2[t] = IloExtractableArray(env, Cut[t].getSize());

            for(IloInt t =0; t<periodos; t++){
                for(IloInt j=0; j<Cut[t].getSize(); j++){
                    integraliza2[t][j] = IloConversion(env, Setup_Troca[t][j], ILOBOOL);
             cutOpt.add(integraliza2[t][j]);
                }
            }



        ///===============================================///
        ///      converte a variavel Cut em inteira       ///
        ///===============================================///
            IloArray<IloExtractableArray> integraliza(env, periodos);
            for(IloInt t =0; t<periodos; t++)
                integraliza[t] = IloExtractableArray(env, Cut[t].getSize());


            for(IloInt t =0; t<periodos; t++){
                for(IloInt j=0; j<Cut[t].getSize(); j++){
                    integraliza[t][j] = IloConversion(env, Cut[t][j], ILOINT);

            cutOpt.add(integraliza[t][j]);
                }

            }

         // fix-and-optimize
    for(t = 0 ; t<periodos; t++){

        ///=============================================///
        ///          fixa a parte inteira               ///
        ///=============================================///
        for(int t2=t+1; t2<periodos; t2++){

            for(IloInt j =0; j<Cut[t2].getSize(); j++){

                Cut[t2][j].setBounds(sol[t2][j], sol[t2][j]);
                Setup_Troca[t2][j].setBounds(setupRel[t2][j], setupRel[t2][j]);

            }
        }


         cplex2.solve();
          ofjfixop = cplex2.getObjValue();

                for(IloInt j =0; j<Cut[t].getSize(); j++){
                    sol[t][j] = cplex2.getValue(Cut[t][j]);  /// guarda a solucao corrente
                    setupRel[t][j] = cplex2.getValue(Setup_Troca[t][j]);  /// guarda a solucao corrente
                }

    }



        cout << " -- Resultado Fix-and-Optimize -- " << ofjfixop  << endl;
        double time_crono3 = crono3.getTime() ;
        std::cout << " time FINAL fop = " << time_crono3  << std::endl;






/*
                std::cout << " X == " << sol << std::endl;
                std::cout << " v == " << vrf << std::endl;
                std::cout << " Y == " << Yrf << std::endl;

                std::cout << " ATRASO == " << AtrasoRel << std::endl;
                std::cout << " ADIANTADO == " << AdiantamentoRel << std::endl;
                std::cout << " ESTOQUE == " << estoqueRel << std::endl;
                std::cout << " TP == " << TempoPreparoRel << std::endl;
                std::cout << " TI == " << TempoInicioRel << std::endl;


*/
        ///========================================================================================================///
        ///                            MATHEURISTICA L0CAL BRANCHING                                               ///
        ///                 recebe o valor encontrado na solucao do relax-and-fix                                  ///
        ///========================================================================================================///
   /*       IloCplex cplexLB(cutOpt);


                cplexLB.setParam(IloCplex::Threads,1);
        		cplexLB.setParam(IloCplex::PreInd, false);

                cplexLB.setOut(env.getNullStream());
                cplexLB.setWarning(env.getNullStream());

                IloNum ObjFOLB = objFOA ;

                bool criterioparada = false;


        ///tamanho inicial da vizinhanca(parametro inteiro positivo)
    int k0 = 20;
    int k = k0;

        /// contador de iterações
            int hmax = 16;
            int h = 1;


            ///=====================================///
            /// DETERMINAR OS VALORES DO ATRASO     ///
            /// ADIANTAMENTO E TEMPO DE PREPARO     ///
            /// APOS O LOCAL BRANCHING              ///
            ///=====================================///

            IloNumArray AtrasoLB(env, periodos);
            IloNumArray AdiantamentoLB(env, periodos);
            IloNumArray TempoPreparoLB(env, periodos);
            IloNumArray TinicioLB(env, periodos);

            IloArray<IloNumArray> estoqueLB(env, periodos);
             for(IloInt t=0; t<periodos; t++)
              estoqueLB[t] = IloNumArray(env,numItens);

            IloArray<IloNumArray> SetupLB(env, periodos); ///limitacao do valor do Cut
             for(IloInt t=0; t<periodos; t++)
               SetupLB[t] = IloNumArray(env, Cut[t].getSize());


        IloArray<IloNumArray> mi(env, periodos); ///limitacao do valor do Cut
        for(IloInt t=0; t<periodos; t++)
          mi[t] = IloNumArray(env, Cut[t].getSize());


        IloArray<IloNumArray> limiteCorte(env, periodos); ///limitacao do valor do Cut
        for(IloInt t=0; t<periodos; t++)
          limiteCorte[t] = IloNumArray(env, Cut[t].getSize());


        for(IloInt t=0; t<periodos; t++){
           for(IloInt j=0; j<Cut[t].getSize(); j++)
               limiteCorte[t][j]  = sol[0][0] ;
            }



        ///=============================================================///
        ///  DETERNINAR MAIOR ELEMENTO DE UMA MATRIZ DADA               ///
        ///=============================================================///
            for(IloInt t=0; t<periodos; t++){
                for(IloInt j=0; j<Cut[t].getSize(); j++){
                    if(limiteCorte[t][j] < sol[t][j]){
                        limiteCorte[t][j] = sol[t][j];
                    }
                  }
                }




        for(IloInt t=0; t<periodos; t++){
          for(IloInt j=0; j<Cut[t].getSize(); j++){
           if(limiteCorte[t][j]!=0){
            mi[t][j] = 1 / limiteCorte[t][j];
          }
         }
        }




        ///===============================================///
        ///      converte a variavel Setup em inteira       ///
        ///===============================================///

           IloArray<IloExtractableArray> integraliza2(env, periodos);
            for(IloInt t =0; t<periodos; t++)
                integraliza2[t] = IloExtractableArray(env, Cut[t].getSize());

            for(IloInt t =0; t<periodos; t++){
                for(IloInt j=0; j<Cut[t].getSize(); j++){
                    integraliza2[t][j] = IloConversion(env, Setup_Troca[t][j], ILOBOOL);
             cutOpt.add(integraliza2[t][j]);
                }
            }



        ///===============================================///
        ///      converte a variavel Cut em inteira       ///
        ///===============================================///
            IloArray<IloExtractableArray> integraliza(env, periodos);
            for(IloInt t =0; t<periodos; t++)
                integraliza[t] = IloExtractableArray(env, Cut[t].getSize());


            for(IloInt t =0; t<periodos; t++){
                for(IloInt j=0; j<Cut[t].getSize(); j++){
                    integraliza[t][j] = IloConversion(env, Cut[t][j], ILOINT);

            cutOpt.add(integraliza[t][j]);
                }

            }

        IloInt TotalLimit = 1200;

        ///======================================================================================================///
        ///                             INICIO DO LOCAL BRANCHING                                                ///
        ///======================================================================================================///

         while(criterioparada == false){

           IloExpr branch(env);/// calculo do branch para a variavel x_tj

   for(IloInt t=0; t<periodos; t++){
                for(IloInt j = 0; j<Cut[t].getSize(); j++){


     if(sol[t][j] == limiteCorte[t][j]){
            branch += mi[t][j]*( limiteCorte[t][j] - Cut[t][j]);
        }
        else if(sol[t][j] == 0 ){
            branch += mi[t][j]* Cut[t][j];
        }

             }
            }





       IloExtractable left_branching = cutOpt.add(  branch <= k);

        ///=============================================///
        ///        CONFIGURACOES DO CPLEX               ///
        ///=============================================///


        cplexLB.setParam(IloCplex::TiLim, TotalLimit  - crono3.getTime()); ///Tempo limite de resolução

        cplexLB.solve();


///==============================================================================================================///

            if (( cplexLB.getStatus() == IloAlgorithm::Optimal
                    or cplexLB.getStatus() == IloAlgorithm::Feasible)
                    and (cplexLB.getObjValue() <   ObjFOLB - EPSILON)){

                for(IloInt t=0; t<periodos; t++){
                    for(IloInt j = 0; j<Cut[t].getSize(); j++){
                        sol[t][j] = cplexLB.getValue(Cut[t][j]);  /// guarda a solucao correte
                        SetupLB[t][j] = cplexLB.getValue(Setup_Troca[t][j]);  /// guarda a solucao correte

                    }

                        AtrasoLB[t] = cplexLB.getValue(Atraso[t]);  /// guarda a solucao correte
                        AdiantamentoLB[t] = cplexLB.getValue(Adiantamento[t]);  /// guarda a solucao correte
                        TempoPreparoLB[t] = cplexLB.getValue(TempoPreparo[t]);  /// guarda a solucao correte

                       for(IloInt i =0; i<numItens; i++)
                        estoqueLB[t][i] =  cplexLB.getValue(Estoque_Itens[t][i]);

                }

                ObjFOLB = cplexLB.getObjValue();
                cutOpt.add(branch >= k +1);
            }



            else if (cplexLB.getStatus() == IloAlgorithm::Infeasible or
                     (cplexLB.getStatus() == IloAlgorithm::Optimal)
                     and (cplexLB.getObjValue() > ObjFOLB - EPSILON)){

                k = k  +  max(1, floor(k/2));

            }
            else{
                k = max(1, floor(k/2));

            }

            cutOpt.remove(left_branching);
            branch.end();
            h++;

          //  cout << " Função " << ObjFOLB << endl;

        if( h >= hmax or crono3.getTime() >  TotalLimit ){
            criterioparada = true;

        } //fim do for periodos
    } //fim do while

        IloNum SomaSetup;
        IloNum SomaCortes;
        IloNum SomaAdiantamentoLB;
        IloNum SomaAtrasoLB ;

  for(IloInt t=0; t<periodos; t++){
    for(IloInt j = 0; j<Cut[t].getSize(); j++){
        SomaSetup += SetupLB[t][j] ;
        SomaCortes += sol[t][j] ;
     }

     SomaAdiantamentoLB += AdiantamentoLB[t];
     SomaAtrasoLB +=  AtrasoLB[t] ;
    }

                  crono3.stop();
                  double time_LB = crono3.getTime() ;


          Local_Branching (cplexLB, periodos, AtrasoLB, AdiantamentoLB, TempoPreparoLB, Cut, sol , ObjFOLB);

                IloExpr objObjetosCortados3(env);
                IloExpr objFOTRLB(env);
                IloExpr objFOARLB(env);
                IloExpr objFOORLB(env);
                IloExpr setups3(env);
                IloExpr objFOTPLB(env);
                IloNum SomaEstoqueLB1 ;
                IloNum SomaEstoqueLB2 ;


  for(IloInt t=0; t<periodos-1; t++){
          for(IloInt i=0; i<numItens; i++)
            SomaEstoqueLB1 += estoqueLB[t][i];
        }

      for(IloInt t=periodos-1; t<periodos; t++){
          for(IloInt i=0; i<numItens; i++){
            SomaEstoqueLB2 += estoqueLB[t][i];
        }
      }

                for(IloInt t=0; t<periodos; t++){
                 for(IloInt j=0; j<Cut[t].getSize(); j++){
                   objObjetosCortados3 += sol[t][j] ;
                   setups3 += SetupLB[t][j];
                 }

                   objFOTRLB += AtrasoLB[t];
                   objFOARLB += AdiantamentoLB[t];
                   objFOTPLB += TempoPreparoLB[t];
                 }

                  cout  << "funçao objetivo - LB - inteira: " << ObjFOLB << endl;
                  env.out() << " Time LB TOTAL = " << (double)crono3.getTime() << endl;
                cout << " Time so LB "   << (double)crono3.getTime() - (double)crono2.getTime()  << endl;

                         IloNum time3 = (double)crono3.getTime() - (double)crono2.getTime()  ;

/*
        cout << " FUNÇÃO " << ObjFOLB << endl;
        cout << " objetos " << objObjetosCortados3 << endl;
        cout << " setups " << setups3 << endl;
        cout << " atraso " << objFOTRLB << endl;
        cout << " adiantado " << objFOARLB << endl;
        cout << " Tempo Preparo " << objFOTPLB << endl;
*/
/*

   cout << "=========================================================================="<<  endl;
         for(IloInt t=0; t<periodos; t++){
           cout << " ====== PERIODO DE TEMPO " << t+1 << " foi cortado " << Cut[t].getSize() << " padroes de corte" << endl;
           cout  << endl;


 for (IloInt j = 0; j <Cut[t].getSize(); j++){
                if(sol[t][j]!=0)

            cout << " Corte "  << sol[t][j] << "  vezes o padrao  " << j+1 <<  endl;
       }

       for (IloInt j = 0; j <Cut[t].getSize(); j++)
         if(sol[t][j]!=0){
            cout << "  Setup Troca "  << SetupLB[t][j]<< "  vezes o padrao  " << j+1 <<  endl;

       }
    }

        cout << "=========================================================================="<<  endl;
         for(IloInt t=0; t<periodos; t++){
           cout << " ====== PERIODO DE TEMPO " <<  t+1 <<  " ===== " << endl;
           cout  << endl;

          cout << " ATRASO  " << t+1 << " = " << AtrasoLB[t] <<   endl;
          cout << " ADIANTAMENTO " << t+1 << " = " <<  AdiantamentoLB[t] <<   endl;
         cout << " TEMPO DE PREPARO do periodo " << t+1 << " = " << TempoPreparoLB[t] <<   endl;
         cout << " TEMPO INICIAL " << t+1 << " = " << TinicioLB[t] <<   endl;

           cout  << endl;
          }
       cout << "=========================================================================="<<  endl;
*/
          //  Local_Branching (cplexLB, periodos, AtrasoLB, AdiantamentoLB, TempoPreparoLB, Cut, sol , ObjFOLB);




                ofstream arquivo1;
                arquivo1.open("F.Inteira-ociosidade(D2-50-3-pesos)", ios::app);
                arquivo1 << MediaNumPadroes ;
                arquivo1 <<  " " ;
                arquivo1 << objFOTRgc ;
                arquivo1 <<  " " ;
                arquivo1 << objFOARgc ;
                arquivo1 <<  " " ;
                arquivo1 <<  setups;
                arquivo1 <<  " " ;
                arquivo1 << objObjetosCortados ;
                arquivo1 <<  " " ;
                arquivo1 << OBJFOGC;
                arquivo1 <<  " " ;
//                arquivo1 << estoque ;
                arquivo1 <<  " " ;
                arquivo1 <<  time_crono;
                arquivo1 <<  "\n " ;
                arquivo1.close();


/*
            ofstream arquivo12;
                arquivo12.open("atraso=SEmArredondameneto-(D3-50-3-p=1)", ios::app);
                arquivo12 <<  objFOad ;
                arquivo12 <<  " " ;
                arquivo12 <<  objFOat;
                arquivo12 <<  " " ;
               arquivo12 <<  setupsar;
                arquivo12 <<  " " ;
                arquivo12 <<  objObjetosCortadosar ;
                arquivo12 <<  " " ;
                arquivo12 <<  objFOA ;
                arquivo12 <<  " " ;
                arquivo12 <<  time2;
                arquivo12 <<  "\n " ;
                arquivo12.close();

/*
                ofstream arquivo12;
                arquivo12.open("FixOptimize-Rfnovo(D3-50-6-100)SEMOciosidade.txt ", ios::app);
//                arquivo12 <<  objFO ;
                arquivo12 <<  " " ;
//                arquivo12 <<  objFOAR ;
                arquivo12 <<  " " ;
//                arquivo12 <<  setups2;
                arquivo12 <<  " " ;
//                arquivo12 <<  objObjetosCortadosR ;
                arquivo12 <<  " " ;
                arquivo12 <<  ofjfixop ;
                arquivo12 <<  " " ;
                arquivo12 <<  time_crono3;
                arquivo12 <<  "\n " ;

                arquivo12.close();

*/

/*
                 ofstream arquivo3;
               arquivo3.open("SEM=LocalBranchingArr-(D3-50-3-p=1).txt ", ios::app);
               arquivo3 << objFOTRLB;
               arquivo3 << " "  ;
               arquivo3 << objFOARLB ;
               arquivo3 << " " ;
               arquivo3 << setups3 ;
               arquivo3 << " "  ;
               arquivo3 << objObjetosCortados3 ;
               arquivo3 << " "  ;
               arquivo3 << ObjFOLB ;
               arquivo3 << " " ;
               arquivo3 <<  time3;
               arquivo3 << "\n " ;
              arquivo3.close();
*/

    }
    catch (IloException& ex)
    {
        cerr << "Erro Cplex: " << ex << endl;
    }
    catch (...)
    {
        cerr << "Erro Cpp:" << endl;
    }
    env.end();
}



int main(){

    char instancia[100];

    for(int i=1; i<=20; i++) {
   // sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Instancias-PCE-DE/D1-140-LARGE-%i.txt ",i);
   // sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Instancias-PCE-DE/TESTE10-%i.txt ",i);
  //  sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Instancias-PCE-DE/TesteDificil-%i.txt ",i);
     //   sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/TESTE-ESTOQUE/D3-SMALL-100-3%i.txt " , i);
   //sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/TESTEPEQUENO", i);
//      sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/D1-P-20-3-%i.txt ",i);
      sprintf(instancia, "/home/prof/Documentos/Elisama-2022/Gerador-2022/D2-50-3-p=1-%i.txt ",i);
   //  sprintf(instancia, "/home/prof/Documentos/Elisama-2022/Gerador-2022/teste3.txt",i);

//      sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/TESTES/DataEntrega-D3/D3-P-100-3-%i.txt ",i);
    //  sprintf(instancia, "/home/elisamaoliveira/Documentos/Classes-Artigo-2021/D1/D1-150-LARGE-%i.txt ",i);
    // sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/D1-50-3-LARGE-%i.txt ",i);
   // sprintf(instancia, "/home/elisamaoliveira/Documentos/Novo_Gerdador_Instancias/D1-LARGE-100-Baixa-%i.txt ",i);
   // sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Isntancias/TesteBase2.txt",i);
//    sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Isntancias/TesteBase.txt",i);
   // sprintf(instancia, "/home/elisamaoliveira/Documentos/Gerador-Isntancias/TESTE-PEQUENO-%i.txt ",i);
        principal(instancia);
    }

    return 0;
}


static void report1 (IloCplex& cutSolver,
                     IloNum& periodos,
                     IloArray<IloNumVarArray> Cut,
                     IloArray<IloRangeArray> Fill)
{

}


static void report2 (IloAlgorithm& patSolver,
                     IloNum& periodos,
                     IloArray<IloNumVarArray> Use,
                     IloObjective obj)
{

}



/** apresenta o resultado da funcao objetivo  **/
static void report3 (IloCplex& cutSolver,
                     IloNum& periodos,
                     IloNum& numItens,
                     IloNumVarArray Atraso,
                     IloNumVarArray Adiantamento,
                     IloNumVarArray TempoPreparo,
                     IloArray<IloNumVarArray> Cut)
{
}





static void Relax_and_Fix(IloCplex& cplex,
                          IloNum& periodos,
                          IloNumArray& AtrasoRel,
                          IloNumArray& AdiantamentoRel,
                          IloNumArray& TempoPreparoRel,
                          IloArray<IloNumVarArray> Cut,
                          IloArray<IloNumArray> sol,
                          IloArray<IloNumArray> SetupRel,
                          IloInt objFO)
{
/*
      cout << endl;
      //cout << "============== SOLUCAO RAF "      << objFO << " OBJETOS ===============" << endl;

        cout << endl;






           cout << "=========================================================================="<<  endl;
             for(IloInt t=0; t<periodos; t++){
               cout << " ====== PERIODO DE TEMPO " <<  t+1 <<  " ===== " << endl;
               cout  << endl;

            for(IloInt j=0; j<Cut[t].getSize(); j++)
              if(SetupRel[t][j] != 0)
                cout << " Setup de Troca de padrão " << j+1 << " -----> " << SetupRel[t][j] << endl;
                cout  << endl;
             }


             cout << "=========================================================================="<<  endl;
             for(IloInt t=0; t<periodos; t++){
               cout << " ====== PERIODO DE TEMPO " <<  t+1 <<  " ===== " << endl;
               cout  << endl;

              cout << " ATRASO  " << t+1 << " = " << AtrasoRel[t] <<   endl;
              cout << " ADIANTAMENTO " << t+1 << " = " <<  AdiantamentoRel[t]<<   endl;
             cout << " TEMPO DE PREPARO do periodo " << t+1 << " = " << TempoPreparoRel[t] <<   endl;

               cout  << endl;
              }
           cout << "=========================================================================="<<  endl;
*/
}


///================================================///
///            LOCAL BRANCHING                     ///
///================================================///
static void Local_Branching (IloAlgorithm&  cplexLB,
                             IloNum& periodos,
                             IloNumArray AtrasoLB,
                             IloNumArray AdiantamentoLB,
                             IloNumArray TempoPreparoLB,
                             IloArray<IloNumVarArray> Cut,
                             IloArray<IloNumArray> sol,
                             IloNum& ObjFOLB){

}


static void Arredondamento(IloCplex& cplex,
                           IloNum& periodos,
                           IloNumArray& AtrasoRel,
                           IloNumArray& AdiantamentoRel,
                           IloNumArray& TempoPreparoRel,
                           IloArray<IloNumVarArray> Cut,
                           IloArray<IloNumArray> sol,
                           IloInt objFO){
/*
           cout << "=========================================================================="<<  endl;
             for(IloInt t=0; t<periodos; t++){
               cout << " ====== PERIODO DE TEMPO " <<  t+1 <<  " ===== " << endl;
               cout  << endl;

            for(IloInt j=0; j<Cut[t].getSize(); j++)
              //  cout << " Setup de Troca de padrão " << j+1 << " -----> " << SetupRel[t][j] << endl;
                cout  << endl;
             }


             cout << "=========================================================================="<<  endl;
             for(IloInt t=0; t<periodos; t++){
               cout << " ====== PERIODO DE TEMPO " <<  t+1 <<  " ===== " << endl;
               cout  << endl;

              cout << " ATRASO  " << t+1 << " = " << AtrasoRel[t] <<   endl;
              cout << " ADIANTAMENTO " << t+1 << " = " <<  AdiantamentoRel[t]<<   endl;
             cout << " TEMPO DE PREPARO do periodo " << t+1 << " = " << TempoPreparoRel[t] <<   endl;

               cout  << endl;
              }
           cout << "=========================================================================="<<  endl;
           */
}
