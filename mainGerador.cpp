#include <ilcplex/ilocplex.h>
#include <time.h>
#include <stdlib.h>

ILOSTLBEGIN

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

typedef IloArray<IloNumArray> IloMatrix2;
typedef IloArray<IloMatrix2> IloMatrix3;

int main(int argc, char **argv){

   IloEnv env;

    try{

        IloModel instancia (env);

  //srand(time(NULL));



 IloInt Comp_Objeto = 1000;
 IloInt numItens = 50;
 IloInt periodos = 3;
 IloInt numInstances = 20;


  IloNumArray Comp_Itens(env, numItens);

  IloNumArray DataEntrega(env, periodos);
  IloNumArray alfa(env, periodos);
  IloNumArray beta(env, periodos);
  IloIntArray entrega(env, periodos);

  IloIntArray f(env, periodos);

  IloIntArray data(env, periodos);

 IloArray<IloIntArray> demanda(env, periodos);
  for(IloInt t=0; t<periodos; t++ )
    demanda[t]= IloIntArray(env, numItens);

      int a1, b1, g1,r1, aux, CMAX;


  IloInt numMaxPadroes = 10000;

  IloInt sobra;
  IloIntArray num_objetos_cortados(env, periodos);
  IloIntArray obj(env, periodos) ;


    IloNumArray objetos(env, numMaxPadroes);


     IloArray<IloNumArray> vetorItensDemandados(env, periodos);
  for(IloInt t=0; t<periodos; t++ )
    vetorItensDemandados[t]= IloNumArray(env, numMaxPadroes);

     IloArray<IloNumArray> vetorItensOrdenados(env, periodos);
  for(IloInt t=0; t<periodos; t++ )
    vetorItensOrdenados[t]= IloNumArray(env, numMaxPadroes);



   IloArray<IloIntArray> sobras(env, periodos);
  for(IloInt t=0; t<periodos; t++ )
    sobras[t]= IloIntArray(env, numMaxPadroes);


      IloNumArray d_total(env, periodos);


    //Criar o vetor de padrão de corte
    IloMatrix3 padraoCorte(env, periodos);
    for (int t = 0; t < periodos; t++) {
      padraoCorte[t] = IloMatrix2(env, numMaxPadroes);
        for (int j = 0; j < numMaxPadroes; j++) {
            padraoCorte[t][j] = IloNumArray(env, numItens);
             for (int i = 0; i < numItens; i++) {
                padraoCorte[t][j][i] = 0;
            }
        }
    }


/***************************************************/
/***      GERADO O COMPRIMENTO DOS ITENS         ***/
/***************************************************/
 for(int r =1; r<=numInstances; r++){  //1 até 7{

    for(IloInt i=0 ; i<numItens; i++){


    // SMALL;
        a1 = floor(0.2*Comp_Objeto);
        b1 = ceil(0.8*Comp_Objeto);

     // MEDIUM;
      // a1 = floor(1.0*Comp_Objeto);
      // b1 = ceil(2.0*Comp_Objeto);

     // LARGE;
      //a1 = floor(0.8*Comp_Objeto);
     // b1 = ceil(1.0*Comp_Objeto);



      do{ // faça
         Comp_Itens[i] =  (a1 - 1)  + (rand() % ( b1  - a1 ) + 1);  /*** GERA UM VETOR DE NUMITENS

   /*****************************************************/
   /**** APRESENTA O VETOR SEM ELEMENTOS REPETIDOS ***** /
   /*****************************************************/

        int repetido = 0;

        for(int j = 0; j < i; j++){ /** percorre a parte do vetor já preenchida **/
            if(Comp_Itens[j] == Comp_Itens[i])
                repetido = 1; /** número repetido **/
        }

        if(repetido == 0) /** significa que o elemento não se repetiu **/
            i++;
       }while(i < numItens); /** enquanto não for sorteado numItens números diferentes */

    }


   /**** ordena o comprimento do vetor de itens ****/
   for(IloInt i=0 ; i<numItens; i++){
       for(IloInt j=i+1 ; j<numItens; j++){
            if(Comp_Itens[i] > Comp_Itens[j]){
               aux = Comp_Itens[i];
               Comp_Itens[i] = Comp_Itens[j];
               Comp_Itens[j] = aux;
            }
        }
    }         cout <<  " Comprimento dos itens :"  << Comp_Itens << endl;








/***************************************************/
/***        GERADO A DEMANDA DOS ITENS           ***/
/***************************************************/
  for(IloInt t=0; t<periodos; t++){
    for(IloInt i=0 ; i<numItens; i++){
      //for(IloInt i=t ; i<numItens; i++){

        a1 = 0;
        b1 = 50;

         demanda[t][i] = (a1 -1)  + (rand() % ( b1  - a1 ) );
         //demanda[i][i] = (a1 )  + (rand() % ( b1  - a1 ) + 1 );
        // demanda[t][i] = 10 ;

      }
                   cout << " demanda = "<< demanda[t] << endl;

    }

    for (int t = 0; t < periodos; t++) {
            d_total[t] = 0;
        for (int i = 0; i < numItens; i++) {
            d_total[t] += demanda[t][i];
        }
         cout << " demanda total = "<< d_total[t] << endl;

    }


/***************************************************/
/***        GERADO A DATA DE ENTREGA             ***/
/***************************************************/
    for(IloInt t=0; t<periodos; t++){
        for(IloInt i=0; i<numItens ; i++){
             //   entrega[t] += demanda[t][i];

                entrega[t] += (Comp_Itens[i]*demanda[t][i]) / Comp_Objeto ;
                }


//

      /*************  D1 *************/
       a1 = ceil(1.5*entrega[t]);
       b1 = ceil(2.0*entrega[t]);
      /*******************************/

      //*************  D2 *************/
       // a1 = floor(1.1*entrega[t]);
      //  b1 = floor(1.2*entrega[t]);
      /*******************************/

      /*************  D3 *************/
      //  a1 = floor(0.8*entrega[t]);
       // b1 = ceil(1.0*entrega[t]);
      /*******************************/
         DataEntrega[t] =  (a1 - 1)  + (rand() % ( b1  - a1 ) + 1);

///==========================================///
///     CONTADOR DE DATA DE ENTREGA          ///
///==========================================///
      if(t==0)
        data[t] = DataEntrega[t] ;
      else
        data[t] = DataEntrega[t] + data[t-1];

  }

       //  cout <<  " Data  : " << DataEntrega << endl;


/***************************************************/
/*** GERADO de panelizações no intervalo dado    ***/
/***************************************************/
         int g1 = 1;
         int peso =1;

         int penalidadeatraso =  g1  + (rand() % peso ) ;
         int penalidadeadiantado =  g1  + (rand() % peso ) ;
         int penalidadef =  g1  + (rand() % peso ) ;


  for(IloInt t=0; t<periodos; t++){

        f[t] = penalidadef;
        alfa[t] = penalidadeatraso    ;
        beta[t] = penalidadeadiantado  ;
  }



/***************************************************/
/***           GERAÇÃO DOS PADROES               ***/
/***************************************************/




    // Passo 1: Criar vetorItensDemandados
              std::cout << std::endl;
  cout << "----------PASSO 1 : Itens DEMANDADOS------------- " << endl;
            std::cout << std::endl;


  for(IloInt t=0; t<periodos; t++){
        int pos = 0;
        for (int i = 0; i < numItens; i++) {
            for (int k = 0; k < demanda[t][i]; k++) {
                vetorItensDemandados[t][pos++] = Comp_Itens[i];
            }
        }
    }

/*
    // Exibir vetorItensDemandados
  for(IloInt t=0; t<periodos; t++){
        std::cout << "Período " << t+1 << ": ";
        for (int j = 0; j < d_total[t]; j++) {
            std::cout << vetorItensDemandados[t][j] << " ";
        }
        std::cout << std::endl;
    }
*/

    // Passo 2: Ordenar vetorItensDemandados
                  std::cout << std::endl;
  cout << "-----------PASSO 2: Itens ORDENADOS-------------- " << endl;
            std::cout << std::endl;

  for(IloInt t=0; t<periodos; t++){
        for (int j = 0; j < d_total[t]; j++) {
            vetorItensOrdenados[t][j] = vetorItensDemandados[t][j];
        }

        // Ordenar o vetor utilizando bubble sort
        for (int j = 0; j < d_total[t]-1; j++) {
            for (int k = 0; k < d_total[t]-j-1; k++) {
                if (vetorItensOrdenados[t][k] < vetorItensOrdenados[t][k+1]) {
                    int temp = vetorItensOrdenados[t][k];
                    vetorItensOrdenados[t][k] = vetorItensOrdenados[t][k+1];
                    vetorItensOrdenados[t][k+1] = temp;
                }
            }
        }
    }

    /*
    // Exibir vetorItensOrdenados
  for(IloInt t=0; t<periodos; t++){
        std::cout << "Período " << t+1 << ": ";
        for (int j = 0; j < d_total[t]; j++) {
            std::cout << vetorItensOrdenados[t][j] << " ";
        }
        std::cout << std::endl;
    }
*/

    // Passo 3: Alocar itens nos objetos
                  std::cout << std::endl;
   cout << "-----------PASSO 3 : HEIRISTICA FFD + Padrao de Corte------------ " << endl;
            std::cout << std::endl;


  for(IloInt t=0; t<periodos; t++){
            num_objetos_cortados[t] = 0;

            std::cout << std::endl;
            std::cout << " Periodo : " << t+1 << std::endl;
            std::cout << std::endl;

        IloInt j = 0;
        while (j < d_total[t]) {

            IloInt item = vetorItensOrdenados[t][j];

             if (sobra >= item) {
                objetos[j] = item;
                sobra -= item;
                sobras[t][ num_objetos_cortados[t]] = sobra;
                std::cout << "Item " << item << " alocado no objeto " <<  num_objetos_cortados[t] << " sobra restante " << sobra << std::endl;
                j++;
            std::cout << std::endl;
            } else {
                num_objetos_cortados[t]++;
                sobra = Comp_Objeto;
            }

        }

        cout  << endl;
        cout  << endl;
  }



    for(IloInt t=0; t<periodos; t++){

    IloInt num_objetos ;
  //  cout  << " Comp_Itens[item - 1] = " ;

    for (int j = 0; j < numItens; j++) {

         IloInt item = vetorItensOrdenados[t][j];

        //  cout  << Comp_Itens[vetorItensOrdenados[t][j]] ;

       bool alocado = false;

       for (int j = 0; j < num_objetos_cortados[t]; j++) {
    //       if (sobra >= Comp_Itens[item - 1] ) {
      //         padraoCorte[t][j][item-1]++;
               sobra -= Comp_Itens[item - 1];
                alocado = true;
                break;
         //  }
        }

        if (!alocado) {
            num_objetos++;
            objetos[num_objetos]++;
            sobra = Comp_Objeto - Comp_Itens[item - 1];
            cout << " Comp_Objeto - Comp_Itens[item - 1] " << Comp_Objeto - Comp_Itens[item - 1];
            padraoCorte[t][num_objetos][item - 1]++;

       }
     }

  }


        cout  << endl;
        cout  << endl;


             cout << " numero de objetos = " << num_objetos_cortados << endl;

  for(IloInt t=0; t<periodos; t++){

    // Exibir padrões de corte
    std::cout << "Periodo " << t + 1 << ":" << std::endl;
    for (int j = 0; j < num_objetos_cortados[t]; j++) {
        std::cout << " padrao = " << j + 1 << "  = ";
       for (int i = 0; i < numItens; i++) {
            std::cout <<  padraoCorte[t][j][i] << " ";
       }

        std::cout << std::endl;
    }
}


/*
           cout << " f = " << f << endl;
           cout << " ENTREGA = " << entrega << endl;
           cout << " CMAX = " << CMAX << endl;
           cout << " alfa = " << alfa << endl;
           cout << " beta = " << beta << endl;
           cout << " demanda = " << demanda << endl;



         cout << " Comprimento do Objeto : " << Comp_Objeto << endl;
         cout << " Comprimento dos Itens : " << Comp_Itens << endl;

         cout <<  " Demanda dos itens :"  << demanda << endl;
         cout <<  " Data  : " << DataEntrega << endl;
         cout <<  " Data de Entrega : " << data << endl;
*/



          ofstream arquivo1;
          char nome_arquivo[FILENAME_MAX];//usado para que a função consiga escrever todos os nomes dentro do char se nao ele nao comporta
          sprintf(nome_arquivo, "TESTE-50-D1-3-\%i.txt ",r); //função que realiza conversao de inteiros para char

        arquivo1.open(nome_arquivo, ios::app);
        arquivo1 <<  Comp_Objeto ;
        arquivo1 <<  "\n " ;
        arquivo1 <<  numItens ;
        arquivo1 <<  "\n " ;
        arquivo1 <<  periodos ;

         for(IloInt i=0 ; i<numItens; i++){
           arquivo1 <<  "\n " ;
           arquivo1 <<  Comp_Itens[i] ;
         }

          for(IloInt t=0; t<periodos; t++){
             arquivo1 <<  "\n " ;
            for(IloInt i=0 ; i<numItens; i++){
              arquivo1 <<  demanda[t][i] ;
              arquivo1 <<  " ";
             }
          }

        arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
             arquivo1 <<  data[t] ;
             arquivo1 <<  " ";
        }

          arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
             arquivo1 <<  alfa[t] ;
             arquivo1 <<  " ";
        }

          arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
             arquivo1 <<  beta[t] ;
             arquivo1 <<  " ";

}
          arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
             arquivo1 <<  f[t] ;
             arquivo1 <<  " ";
        }

          arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
          arquivo1 << num_objetos_cortados[t];
          arquivo1 <<  " ";
        }

          arquivo1 <<  "\n " ;
        for(IloInt t=0; t<periodos; t++){
            for (int j = 0; j < num_objetos_cortados[t]; j++) {
         for(IloInt k=0; k<numItens; k++){
             arquivo1 <<  padraoCorte[t][j][k] ;
                       arquivo1 <<  " ";
           }
          arquivo1 <<  "\n " ;
          }
        }


        arquivo1 <<  "\n " ;
        arquivo1 <<  "\n " ;
        arquivo1.close();


 }

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

        return 0;
}
