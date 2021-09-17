public interface Function1<R, T1>
{
    R invoke( T1 argument1 );
}

public interface Procedure1<T1>
{
    void invoke( T1 argument1 );
}

public static <T> void dump( T node, Function1<List<T>,T> breeder,
       Function1<String,T> stringizer, Procedure1<String> emitter )
{
    emitter.invoke( stringizer.invoke( node ) );
    dumpRecursive( node, "", breeder, stringizer, emitter );
}

private static final String[][] PREFIXES = { { " ├─ ", " │  " }, { " └─ ", "    " } };

private static <T> void dumpRecursive( T node, // tree to print
		String parentPrefix, // pass through
        Function1<List<T>,T> breeder, // Node -> List<Node>. probably gets iterator to all childs
		Function1<String,T> stringizer, // Takes node, returns stringified version
        Procedure1<String> emitter ) // function that actually prints the resulting string
{
    for( Iterator<T> iterator = breeder.invoke( node ).iterator(); iterator.hasNext(); ) // iterate through all children
    {
        T childNode = iterator.next(); // advance to next child
        String[] prefixes = PREFIXES[iterator.hasNext()? 0 : 1]; // adjust prefix strings based on child ordering
        emitter.invoke( parentPrefix + prefixes[0] + stringizer.invoke( childNode ) ); // print child
        dumpRecursive( childNode, parentPrefix + prefixes[1], breeder, stringizer, emitter ); // recursive
    }
}