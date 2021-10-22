package learnLib;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.StringWriter;
import java.sql.Array;
import java.util.ArrayList;
import java.util.Arrays;
import de.learnlib.api.algorithm.PassiveLearningAlgorithm;
import de.learnlib.api.query.DefaultQuery;
import de.learnlib.api.query.Query;
import de.learnlib.datastructure.observationtable.OTUtils;
import de.learnlib.datastructure.observationtable.writer.ObservationTableASCIIWriter;
import de.learnlib.datastructure.pta.pta.BlueFringePTA;
import net.automatalib.automata.transducers.MealyMachine;
import net.automatalib.automata.transducers.impl.compact.CompactMealy;
import net.automatalib.commons.util.Pair;
import net.automatalib.serialization.dot.GraphDOT;
import net.automatalib.words.Alphabet;
import net.automatalib.words.Word;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Scanner;

import de.learnlib.algorithms.rpni.AbstractBlueFringeRPNI;
import de.learnlib.algorithms.rpni.BlueFringeRPNIDFA;
import de.learnlib.algorithms.rpni.BlueFringeRPNIMealy;
import de.learnlib.api.algorithm.PassiveLearningAlgorithm.PassiveDFALearner;
import de.learnlib.api.algorithm.PassiveLearningAlgorithm.PassiveMealyLearner;
import net.automatalib.automata.fsa.DFA;
import net.automatalib.visualization.Visualization;
import net.automatalib.words.impl.Alphabets;


public final class passive_Mealy_simulationA {

    private passive_Mealy_simulationA() {
        // prevent instantiation
    }

    public static void main(String[] args) throws IOException {

        // define the alphabet
    /*
    	Alphabet<String> alphabet = Alphabets.fromArray("Delta_goto_ki",
    			"Delta_getStatus_ki_run",
    			"Delta_getStatus_ki_suc",
    			"Delta_getStatus_ki_ab",
    			"Delta_halt_ki",
    			"Delta_goto_ch",
    			"Delta_getStatus_ch_run",
    			"Delta_getStatus_ch_suc",
    			"Delta_getStatus_ch_ab",
    			"Delta_batteyStatus_false",
    			"Delta_batteyStatus_true",
    			"Delta_isAt_ki_false",
    			"Delta_isAt_ki_true",
    			"Delta_isAt_ch_false",
    			"Delta_isAt_ch_true",
    			"Delta_halt_ch",
    			"Delta_level_high",
    			"Delta_level_medium",
    			"Delta_level_low",
    			"level_i",
    			"getStatus_ki_i",
    			"halt_ki_i",
    			"goTo_ki_i",
    			"goTo_ch_i",
    			"getStatus_ch_i",
    			"batteryStatus_i",
    			"isAt_ki_i",
    			"isAt_ch_i",
    			"halt_ch_i");
*/
         Alphabet<String> alphabet = Alphabets.fromArray("Delta","goTo_ki_i","level_i","getStatus_ki_i","halt_ki_i","goTo_ch_i","getStatus_ch_i","batteryStatus_i","isAt_ki_i","isAt_ch_i","halt_ch_i");

        // if no training samples have been provided, only the empty automaton can be constructed
        final MealyMachine<?, String, ?, String> emptyModel = computeModel(alphabet, Collections.emptyList());;
        Visualization.visualize(emptyModel, alphabet);

        // since RPNI is a greedy state-merging algorithm, providing only positive examples results in the trivial
        // one-state acceptor, because there exist no negative "counterexamples" that prevent state merges when
        // generalizing the initial prefix tree acceptor
        final MealyMachine<?, String, ?, String>  firstModel =
                computeModel(alphabet,get_traces());
        Visualization.visualize(firstModel, alphabet);
       
        // with negative samples (i.e. words that must not be accepted by the model) we get a more "realistic"
        // generalization of the given training set
        
    }

   
    
    private static Collection < ? extends DefaultQuery<String, Word<String>>> get_traces() throws IOException  {
    	
    	List<Scanner> s = new ArrayList<Scanner>();
    	List<Scanner> s1 = new ArrayList<Scanner>();
    	
    	List <ArrayList<String>> input_pre = new ArrayList<ArrayList<String>>();
    	List<ArrayList<String>> output = new ArrayList<ArrayList<String>>();
    	
    	for (int i = 0; i < 4; i++) {
    		
    		s.add(i,new Scanner(new File("simulatorA/input_simulator_nominal"+(i+1)+".txt")));
    		ArrayList<String> input_list = new ArrayList<String>();
    		while (s.get(i).hasNext()){
        		
        		input_list.add(s.get(i).next());
        		input_pre.add(i,input_list);
        	}
        	s.get(i).close();
        	
        	
        	s1.add(new Scanner(new File("simulatorA/output_simulator_nominal"+(i+1)+".txt")));
        	ArrayList<String> output_list = new ArrayList<String>();
        	while (s1.get(i).hasNext()){
        		
        		output_list.add(s1.get(i).next());
        		output.add(i,output_list);
        	}
        	s1.get(i).close();
    		
    	}
    	
    	
    	
    	return Arrays.asList( new DefaultQuery<>(Word.fromList(input_pre.get(0)),Word.fromSymbols("Delta"),Word.fromList(output.get(0))),
    			new DefaultQuery<>(Word.fromList(input_pre.get(1)),Word.fromSymbols("Delta"),Word.fromList(output.get(1))),
    			new DefaultQuery<>(Word.fromList(input_pre.get(2)),Word.fromSymbols("Delta"),Word.fromList(output.get(2))),
    			new DefaultQuery<>(Word.fromList(input_pre.get(3)),Word.fromSymbols("Delta"),Word.fromList(output.get(3)))
    			/*new DefaultQuery<>(Word.fromList(input_pre.get(4)),Word.fromSymbols("Delta"),Word.fromList(output.get(4))),
    			new DefaultQuery<>(Word.fromList(input_pre.get(5)),Word.fromSymbols("Delta"),Word.fromList(output.get(5)))
    			*/
    			
    			
    			);
    	
    	/*
    	Scanner s = new Scanner(new File("simulator/input_simulator_nominal.txt"));
    	ArrayList<String> input_pre = new ArrayList<String>();
    	while (s.hasNext()){
    	    input_pre.add(s.next());
    	}
    	s.close();
    	
    	Scanner s2 = new Scanner(new File("simulator/output_simulator_nominal.txt"));
    	ArrayList<String> output = new ArrayList<String>();
    	while (s2.hasNext()){
    	    output.add(s2.next());
    	}
    	s2.close();
    	*/
    }

    /**
     * Returns the negative samples from the example of chapter 12.4.3 of "Grammatical Inference" by Colin de la
     * Higuera.
     *
     * @return a collection of (negative) training samples
     */
    
    /**
     * Creates the learner instance, computes and return the inferred model.
     *
     * @param alphabet
     *         domain from which the learning data are sampled
     * @param positiveSamples
     *         positive samples
     * @param negativeSamples
     *         negative samples
     * @param <I>
     *         input symbol type
     *
     * @return the learned model
     * @throws IOException 
     */
    private static <I,O> MealyMachine<?, I, ?, O> computeModel(Alphabet<I> alphabet,Collection <? extends DefaultQuery<I, Word<O>>> traces) throws IOException {

    	
        
    	final PassiveMealyLearner<I,O> learner = new BlueFringeRPNIMealy<>(alphabet);

        //learner.addSamples(samples);
        
    	learner.addSamples(traces);
    	((AbstractBlueFringeRPNI<I, Word<O>, Void, O, MealyMachine<?, I, ?, O>>) learner).setParallel(true);
    	((AbstractBlueFringeRPNI<I, Word<O>, Void, O, MealyMachine<?, I, ?, O>>) learner).setDeterministic(true);
        
        return learner.computeModel();
        
        

    }
}
