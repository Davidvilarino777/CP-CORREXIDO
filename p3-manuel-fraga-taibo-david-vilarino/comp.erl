-module(comp).

-export([comp_loop_proc/3, comp/1, comp/2 , comp_proc/2, comp_proc/3,decomp_loop_proc/3, decomp_proc/2, decomp_proc/3, decomp/1, decomp/2]).


-define(DEFAULT_CHUNK_SIZE, 1024*1024).

%%% File Compression
comp_loop(Reader, Writer) ->  %% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop(Reader, Writer);
        eof ->  %% end of file, stop reader and writer
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.
comp(File) -> %% Compress file to file.ch
    comp(File, ?DEFAULT_CHUNK_SIZE).

comp(File, Chunk_Size) ->  %% Starts a reader and a writer which run in separate processes
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    comp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

comp_loop_proc(Reader, Writer, FROM) ->  %% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop_proc(Reader, Writer, FROM);
        eof ->  %% end of file, stop reader and writer
            FROM ! {'EXIT', self(), "Terminado"};
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.

comp_proc(File, Procs) ->
    comp_proc(File, ?DEFAULT_CHUNK_SIZE, Procs).

comp_proc(File, Chunk_Size, Procs)->
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    case create(Procs, Reader, Writer)  of
                        created-> wait(Procs)
                    end;
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

create(0, _, _)-> created;
create(Procs, Reader, Writer) when is_integer(Procs), Procs>0 -> 
    spawn(comp, comp_loop_proc,  [Reader, Writer, self ()]),
    create(Procs-1, Reader, Writer);
create(Procs, _, _)-> 
    io:format("Invalid argument for wait function: ~p~n", [Procs]).

wait(0) -> io:format("All processes finished.~n");

wait(Procs) when is_integer(Procs), Procs > 0 -> 
    receive
        {'EXIT', From, Reason} ->
            io:format("Proceso ~p terminó con motivo: ~p~n", [From, Reason])
    end,
    wait(Procs-1).



%% File Decompression

decomp(Archive) ->
    decomp(Archive, string:replace(Archive, ".ch", "", [trailing])).

decomp(Archive, Output_File) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    decomp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

decomp_loop(Reader, Writer) ->
    Reader ! {get_chunk, self()},  %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop(Reader, Writer);
        eof ->    %% end of file => exit decompression
            Reader ! stop,
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.

decomp_proc(Archive, Procs) ->
    decomp_proc(Archive, string:replace(Archive, ".ch", "", [trailing]), Procs).

decomp_proc(Archive, Output_File, Procs) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    case createdecomp(Procs, Reader, Writer) of
                        created-> wait(Procs)
                    end;
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

decomp_loop_proc(Reader, Writer,FROM) ->
    Reader ! {get_chunk, self()},  %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop_proc(Reader, Writer,FROM);
        eof ->    %% end of file => exit decompression
            FROM ! {'EXIT', self(), "Terminado"};
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.
createdecomp(0, _, _)-> created;
createdecomp(Procs, Reader, Writer) when is_integer(Procs), Procs>0 -> 
    spawn(comp, decomp_loop_proc,  [Reader, Writer, self ()]),
    createdecomp(Procs-1, Reader, Writer);
createdecomp(Procs, _, _)-> 
    io:format("Invalid argument for wait function: ~p~n", [Procs]).